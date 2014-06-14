#ifndef JPEG_UTILS_H
#define JPEG_UTILS_H

/******************************************************************************
Description.: jpeg types to detect
******************************************************************************/

#define JPEG_GOOD_REGULAR 0
#define JPEG_GOOD_PROGRESSIVE 1
#define JPEG_BAD 2
/******************************************************************************
Description.: check incoming JPEG type
Input Value.:
    unsigned char* jpg     : jpeg to check
    unsigned long jpgSize  : jpeg size
Return Value:
    one of JPEG_XXX constants (see above)
******************************************************************************/
int ju_checkIfJpeg(unsigned char* jpg, unsigned long jpgSize);




/******************************************************************************
Description.: re-encoding from plain to progressive
Input Value.: 
	char* inJpg     : jpeg to re-encode
	long inJpgSize  : jpeg size
Input-output Value.: 
	char** outJpg   : buffer for re-encoded jpeg
	long* outJpgSize: buffer for re-encoded jpeg size
Return Value:
	TRUE on success
******************************************************************************/

 int ju_processFrame(unsigned char* inJpg, unsigned long inJpgSize, unsigned char** outJpg, unsigned long* outJpgSize);

/******************************************************************************
Description.: cropping image to fit network channel 
Input Value.: 
	char* jpg       : jpeg to crop. IMPORTANT - function replaces EOI marker
	long jpgSize    : jpeg size
	long maxCropSize: maximum allowed size (may be overrunned if unable to crop)
Return Value:
	size of cropped image
******************************************************************************/
unsigned long ju_cropJpeg (unsigned char* jpg, unsigned long jpgSize, unsigned long maxCropSize);


/******************************************************************************
Description.: initial alloc
******************************************************************************/

void ju_prepare();

/******************************************************************************
Description.: Resource deallocation in case of emergency. Use in pthread_cleanup()
and similar
******************************************************************************/
 void ju_cleanup();


#endif
