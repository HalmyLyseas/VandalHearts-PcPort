/* CD-XA ADPCM streaming (music) backend. See src/pc_xa.c. Driven by libcd.c's sector pump
 * (which owns the disc file) and serviced once per VSync. */
#ifndef PLATFORM_PC_PC_XA_H
#define PLATFORM_PC_PC_XA_H
void PC_XaReset(void);                                   /* stop + clear ADPCM history      */
void PC_XaSetVolume(int left, int right);                /* CD serial volume 0..127/side    */
int  PC_XaSubmitSector(const unsigned char *raw2352, int wantFile, int wantChan);
int  PC_XaQueuedBuffers(void);                           /* in-flight buffers (flow control) */
void PC_XaService(void);                                 /* VSync: recycle buffers, keep playing */
void PC_XaEndStream(void);                               /* track data ended: stop auto-restart  */
void PC_CdXaUpdate(void);                                /* libcd.c pump; call once per VSync    */
#endif
