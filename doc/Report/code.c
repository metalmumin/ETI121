#include <std.h>
#include <math.h>

#include <hst.h>
#include <log.h>
#include <pip.h>
#include <swi.h>
#include <sys.h>

#include <iom.h>
#include <pio.h>

#ifdef _6x_
extern far LOG_Obj trace;
extern far PIP_Obj pipRx;
extern far PIP_Obj pipTx;
extern far SWI_Obj swiEcho;
#else
extern LOG_Obj trace;
extern PIP_Obj pipRx;
extern PIP_Obj pipTx;
extern SWI_Obj swiEcho;
#endif

#define FILTER_LEN 700		// must be divisible by 2
#define AUDIO_LEN  256
#define BUFF_LEN AUDIO_LEN/2
short oldInData[FILTER_LEN] = { 0 };

float short_w[FILTER_LEN];
short oldInPos = 0;
float norm_u = 0;
double var_tot = 0;
float tot_num = 0;

void mexNewNLMS(size_t N_LEN, short *in, short *err, double mu, double a);

/*
 *  'pioRx' and 'pioTx' objects will be initialized by PIO_new(). 
 */
PIO_Obj pioRx, pioTx;

/*
 *  ======== main ========
 *
 *  Application startup funtion called by DSP/BIOS. Initialize the
 *  PIO adapter then return back into DSP/BIOS.
 */
main()
{

  int indx;
  for (indx = 0; indx < FILTER_LEN; indx++) {

    short_w[indx] = 0;
    oldInData[indx] = 0;
  }

  /*
   * Initialize PIO module
   */
  PIO_init();



  /* Bind the PIPs to the channels using the PIO class drivers */
  PIO_new(&pioRx, &pipRx, "/udevCodec", IOM_INPUT, NULL);
  PIO_new(&pioTx, &pipTx, "/udevCodec", IOM_OUTPUT, NULL);

  /*
   * Prime the transmit side with buffers of silence.
   * The transmitter should be started before the receiver.
   * This results in input-to-output latency being one full
   * buffer period if the pipes is configured for 2 frames.
   */
  PIO_txStart(&pioTx, PIP_getWriterNumFrames(&pipTx), 0);

  /* Prime the receive side with empty buffers to be filled. */
  PIO_rxStart(&pioRx, PIP_getWriterNumFrames(&pipRx));

  LOG_printf(&trace, "pip_audio started");
}


/*
 *  ======== echo ========
 *
 *  This function is called by the swiEcho DSP/BIOS SWI thread created
 *  statically with the DSP/BIOS configuration tool. The PIO adapter
 *  posts the swi when an the input PIP has a buffer of data and the
 *  output PIP has an empty buffer to put new data into. This function
 *  copies from the input PIP to the output PIP. You could easily
 *  replace the copy function with a signal processing algorithm.
 */
Void echo(Void)
{
  Int audiosize;
  short *audiosrc, *audiodst;

  /*
   * Check that the precondions are met, that is pipRx has a buffer of
   * data and pipTx has a free buffer.
   */
  if (PIP_getReaderNumFrames(&pipRx) <= 0) {
    LOG_error("echo: No reader frame!", 0);
    return;
  }
  if (PIP_getWriterNumFrames(&pipTx) <= 0) {
    LOG_error("echo: No writer frame!", 0);
    return;
  }

  /* get the full buffer from the receive PIP */
  PIP_get(&pipRx);
  audiosrc = PIP_getReaderAddr(&pipRx);
  audiosize = PIP_getReaderSize(&pipRx) * sizeof(short);

  /* get the empty buffer from the transmit PIP */
  PIP_alloc(&pipTx);
  audiodst = PIP_getWriterAddr(&pipTx);


  /* Do the data processing. */
  //    process(audiosrc,audiosize,audiodst);    

  //mexNewNLMS( audiosrc,  audiodst, 0.5, 0.01, audiosrc);


  mexNewNLMS(audiosize, audiosrc, audiodst, 0.5, 0.01);

  /* Record the amount of actual data being sent */
  PIP_setWriterSize(&pipTx, PIP_getReaderSize(&pipRx));

  /* Free the receive buffer, put the transmit buffer */
  PIP_put(&pipTx);
  PIP_free(&pipRx);
}

/*
 *  ======== copy ========
 * 
 *  This function is called by the swiCopy DSP/BIOS SWI thread created
 *  statically with the DSP/BIOS configuration tool. The PIO adapter
 *  posts the swi when an the input HST has a buffer of data and the
 *  output HST has an empty buffer to put new data into. This function
 *  copies from the input HST to the output HST. You could easily
 *  replace the copy function with a signal processing algorithm.
 */

Void copy(HST_Obj * input, HST_Obj * output)
{
  PIP_Obj *in, *out;
  Uns *datasrc, *datadst;
  Uns datasize;
  short i;

  in = HST_getpipe(input);
  out = HST_getpipe(output);

  if (PIP_getReaderNumFrames(in) == 0 || PIP_getWriterNumFrames(out) == 0) {
    LOG_error("echo: No data to read from file!", 0);
  }


  LOG_printf(&trace, "in copy");
  /* get input data and allocate output buffer */
  PIP_get(in);
  PIP_alloc(out);

  /* copy input data to output buffer */
  datasrc = PIP_getReaderAddr(in);
  datadst = PIP_getWriterAddr(out);

  datasize = PIP_getReaderSize(in);
  PIP_setWriterSize(out, datasize);

  for (i = 0; i < datasize; i++) {
    *datadst++ = *datasrc++;
  }

  /* output copied data and free input buffer */
  PIP_put(out);
  PIP_free(in);
}

short inline readEcho(short *data, int index)
{
  return data[index * 2 + 1];
}

short inline readClean(short *data, size_t index)
{
  return data[index * 2];
}

void writeStereo(short value, short *dest, int index)
{
  dest[index * 2] = value;
  dest[index * 2 + 1] = value;
}


short inline readOffset(size_t offset, short *oldInData, size_t index)
{
  offset = (index + offset) % FILTER_LEN;
  return oldInData[offset];
}

void mexNewNLMS(size_t N_LEN, short *in, short *err, double mu, double a)
{
  size_t t;
  double w_mul_in;
  double a_norm_u, temp;
  size_t buf_len = N_LEN / 2;
  short m;
  short value;

  double curDataSq;
  double var_buf = 0;
  short buf_num = 0;
  double varb, vart;

  // Matlab
  // uvec=u(n-Delay:-1:n-Delay-M+1);
  // algorithm, using, a, mu, flen
  for (t = 0; t < buf_len; ++t) {
    curDataSq = readClean(in, t) * readClean(in, t);
    var_buf += curDataSq;
    buf_num++;

    var_tot += curDataSq;
    tot_num++;
    varb = var_buf / buf_num;
    vart = var_tot / tot_num;


    norm_u += curDataSq;
    norm_u -= oldInData[t] * oldInData[t];

    // MATLAB:          e(n)=d(n)-w'*uvec;
    w_mul_in = 0;		//reset for this iteration
    
    //calculate the echo for this sample
    for (m = 0; m <= t; ++m) {
      w_mul_in += short_w[m] * readClean(in, (t - m));
    }

    for (m = t + 1; m < FILTER_LEN; ++m) {
      w_mul_in += short_w[m] * oldInData[FILTER_LEN + t - m];
    }

    temp = readEcho(in, t) - w_mul_in;


    err[t * 2] = temp;
    err[1 + t * 2] = (short) temp;


    if (varb > vart) {

      a_norm_u = (mu / (a + norm_u)) * temp;
      for (m = 0; m <= t; ++m) {
	short_w[m] += a_norm_u * readClean(in, (t - m));
      }

      for (m = t + 1; m < FILTER_LEN; ++m) {
	short_w[m] += a_norm_u * oldInData[FILTER_LEN + t - m];
      }

    }

  }

  for (m = 0; m < FILTER_LEN; ++m) {
    if (m < (FILTER_LEN - buf_len)) {
      value = oldInData[buf_len + m];	//move old values forward
    } else {
      value = readClean(in, (m - (FILTER_LEN - buf_len)));	//append new data
      oldInData[m] = value;
    }
  }
}
