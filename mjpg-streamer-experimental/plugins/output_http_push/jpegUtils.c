#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <jpeglib.h>
#include <string.h> // memcpy
#include "jpegUtils.h"


struct jpeg_compress_struct cinfo;
struct jpeg_decompress_struct dinfo;

struct jpeg_error_mgr       jcerr,jderr;
 

jvirt_barray_ptr * data = NULL;

typedef struct {
    struct jpeg_destination_mgr pub; /* public fields */

    JOCTET * buffer;    /* start of buffer */

    unsigned char *outbuffer;
    int outbuffer_size;
    unsigned char *outbuffer_cursor;
    int *written;

} mjpg_destination_mgr;

typedef mjpg_destination_mgr * mjpg_dest_ptr;

/******************************************************************************
Description.:
Input Value.:
Return Value:
******************************************************************************/
METHODDEF(void) init_destination(j_compress_ptr cinfo)
{
    mjpg_dest_ptr dest = (mjpg_dest_ptr) cinfo->dest;

    /* Allocate the output buffer --- it will be released when done with image */
    dest->buffer = (JOCTET *)(*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_IMAGE, OUTPUT_BUF_SIZE * sizeof(JOCTET));

    *(dest->written) = 0;

    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

/******************************************************************************
Description.: called whenever local jpeg buffer fills up
Input Value.:
Return Value:
******************************************************************************/
METHODDEF(boolean) empty_output_buffer(j_compress_ptr cinfo)
{
    mjpg_dest_ptr dest = (mjpg_dest_ptr) cinfo->dest;

    memcpy(dest->outbuffer_cursor, dest->buffer, OUTPUT_BUF_SIZE);
    dest->outbuffer_cursor += OUTPUT_BUF_SIZE;
    *(dest->written) += OUTPUT_BUF_SIZE;

    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;

    return TRUE;
}

/******************************************************************************
Description.: called by jpeg_finish_compress after all data has been written.
              Usually needs to flush buffer.
Input Value.:
Return Value:
******************************************************************************/
METHODDEF(void) term_destination(j_compress_ptr cinfo)
{
    mjpg_dest_ptr dest = (mjpg_dest_ptr) cinfo->dest;
    size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;

    /* Write any data remaining in the buffer */
    memcpy(dest->outbuffer_cursor, dest->buffer, datacount);
    dest->outbuffer_cursor += datacount;
    *(dest->written) += datacount;
}

/******************************************************************************
Description.: Prepare for output to a stdio stream.
Input Value.: buffer is the already allocated buffer memory that will hold
              the compressed picture. "size" is the size in bytes.
Return Value: -
******************************************************************************/
GLOBAL(void) dest_buffer(j_compress_ptr cinfo, unsigned char *buffer, int size, int *written)
{
    mjpg_dest_ptr dest;

    if(cinfo->dest == NULL) {
        cinfo->dest = (struct jpeg_destination_mgr *)(*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(mjpg_destination_mgr));
    }

    dest = (mjpg_dest_ptr) cinfo->dest;
    dest->pub.init_destination = init_destination;
    dest->pub.empty_output_buffer = empty_output_buffer;
    dest->pub.term_destination = term_destination;
    dest->outbuffer = buffer;
    dest->outbuffer_size = size;
    dest->outbuffer_cursor = buffer;
    dest->written = written;
}

void finishDecoding(void){

	jpeg_destroy_decompress(&dinfo);

}
void finishEncoding(void){

	jpeg_destroy_compress(&cinfo);
}



void ju_prepare()
{


    dinfo.err = jpeg_std_error(&jderr);
    jpeg_create_decompress(&dinfo);
    cinfo.err = jpeg_std_error(&jcerr);
    jpeg_create_compress(&cinfo);

    //2 MB to both
    cinfo.mem->max_memory_to_use = 2*1024*1024;
    dinfo.mem->max_memory_to_use = 2*1024*1024;
}



void prepareDecoding(unsigned char* inJpg, unsigned long inJpgSize)
{

	//dinfo.raw_data_out=TRUE;
	//printf ("here1");
	jpeg_mem_src(&dinfo,inJpg,inJpgSize);
	//jpeg_start_decompress(&dinfo);
	//printf ("here");
	jpeg_read_header(&dinfo,TRUE);
 	data = jpeg_read_coefficients(&dinfo);
	//printf ("pass");
}

void prepareEncoding(unsigned char* outJpgBuffer, int outJpgBufSize,int* written)
{




	
	//jpeg_set_quality(&cinfo, 80, TRUE); //todo upgrade

	jpeg_copy_critical_parameters(&dinfo, &cinfo);
	//jpeg_set_defaults(&cinfo);
	jpeg_simple_progression(&cinfo);
  	//cinfo.image_width = dinfo.image_width; 	/* image width and height, in pixels */
  	//cinfo.image_height = dinfo.image_height;
  	//cinfo.input_components = dinfo.input_components;/* # of color components per pixel */
  	//cinfo.in_color_space = dinfo.in_color_space; 	/* colorspace of input image */

	//cinfo.dct_method = JDCT_FASTEST; 
	
	//cinfo.raw_data_in = TRUE; // Supply downsampled data 

    dest_buffer(&cinfo, outJpgBuffer, outJpgBufSize, written);
    //jpeg_mem_dest(&cinfo,outJpg,outJpgSize);
	//jpeg_start_compress(&cinfo, TRUE);
	//jpeg_write_header(&cinfo);
	//printf ("pass2");
 	
}

void copyData()
{
	jpeg_write_coefficients(&cinfo,data);
}



int ju_processFrame(unsigned char* inJpg, unsigned long inJpgSize, unsigned char* outJpgBuf,  int outJpgBufSize)
{
    static int written;
	prepareDecoding(inJpg,inJpgSize);
    written=0;// just in case
    prepareEncoding(outJpgBuf,outJpgBufSize,&written);
	
	copyData();


    // should be first to copy coefficients data
    jpeg_finish_compress(&cinfo);
    jpeg_finish_decompress(&dinfo);

    return written;
}

#define MARKER_START ((unsigned char)0xFF)
#define MARKER_EOI ((unsigned char)0xD9)
#define MARKER_SOS ((unsigned char)0xDA)
#define MARKER_REGULAR ((unsigned char)0xC0)
#define MARKER_REGULAR_EXT ((unsigned char)0xC1)
#define MARKER_PROGRESSIVE ((unsigned char)0xC2)


unsigned long ju_cropJpeg (unsigned char* jpg, unsigned long jpgSize, unsigned long maxCropSize)
{
	if(maxCropSize>=jpgSize)
	{
		return jpgSize;
	}
	
	int sectionStart=0;
	int prevSectionStart=0;
	


	for(int i=0;i<maxCropSize-1;i++)
	{	
		if(jpg[i]==MARKER_START)
            	{ 
			switch(jpg[i+1]){
			//what if...
			case MARKER_EOI:
				// end of JPEG found!
		  		return i+1;
			case MARKER_SOS:
				prevSectionStart=sectionStart;
				sectionStart=i;
			}

            	}
	}
	if(sectionStart==0)
	{
		// not a progressive jpeg, unable to crop
		return jpgSize;
	}

	// if we are here, jpeg is bigger than needed
	if((prevSectionStart==0) && (sectionStart!=0) )// we are still in first section
	{
		// nothing to crop, scanning to next section
		bool sectionFound = FALSE;				
		for(int i=maxCropSize-1;i<jpgSize-1;i++)
		{	
			
			if(jpg[i]==MARKER_START)
            		{ 
				switch(jpg[i+1]){
				//what if...
				case MARKER_EOI:
					// end of JPEG found!
		  			return i+1;
				case MARKER_SOS:
					prevSectionStart=sectionStart;
					sectionStart=i;
					sectionFound = TRUE;
				}

            		}
			if(sectionFound) break;
		}
		
		
	}

	// now we have start and end of last good section
	// replacing next SOS marker with EOI marker
	jpg[sectionStart+1]=MARKER_EOI;
	return sectionStart+1;
	
}



//#define MARKER_REGULAR ((unsigned char)0xC0)
//#define MARKER_REGULAR_EXT ((unsigned char)0xC1)
//#define MARKER_PROGRESSIVE ((unsigned char)0xC2)


int ju_checkIfJpeg(unsigned char* jpg, unsigned long jpgSize)
{
    for(int i=0;i<jpgSize-1;i++)
    {
        if(jpg[i]==MARKER_START)
        {
            switch(jpg[i+1]){
            //what if...
            case MARKER_REGULAR:
            case MARKER_REGULAR_EXT:
                // regular
                return JPEG_GOOD_REGULAR;
            case MARKER_PROGRESSIVE:
                return JPEG_GOOD_PROGRESSIVE;
            }

        }
    }
    return JPEG_BAD;
}





void ju_cleanup()
{
	jpeg_destroy_decompress(&dinfo);
	jpeg_destroy_compress(&cinfo);
}
