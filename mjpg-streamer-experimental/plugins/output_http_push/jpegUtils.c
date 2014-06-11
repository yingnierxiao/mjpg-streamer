#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <jpeglib.h>
#include "jpegUtils.h"


struct jpeg_compress_struct cinfo;
struct jpeg_decompress_struct dinfo;

struct jpeg_error_mgr       jerr;
 

jvirt_barray_ptr * data = NULL;


void finishDecoding(void){
	jpeg_finish_decompress(&dinfo);
	jpeg_destroy_decompress(&dinfo);

}
void finishEncoding(void){
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
}




void prepareDecoding(unsigned char* inJpg, unsigned long inJpgSize)
{
	dinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&dinfo);
	//dinfo.raw_data_out=TRUE;
	//printf ("here1");
	jpeg_mem_src(&dinfo,inJpg,inJpgSize);
	//jpeg_start_decompress(&dinfo);
	//printf ("here");
	jpeg_read_header(&dinfo,TRUE);
 	data = jpeg_read_coefficients(&dinfo);
	//printf ("pass");





}

void prepareEncoding(unsigned char** outJpg, unsigned long* outJpgSize)
{


	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo); 	
	

	
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
	jpeg_mem_dest(&cinfo,outJpg,outJpgSize);
	//jpeg_start_compress(&cinfo, TRUE);
	//jpeg_write_header(&cinfo);
	//printf ("pass2");
 	
}

void copyData()
{
	/*int readLines = dinfo.max_v_samp_factor*DCTSIZE;
	int cc =dinfo.out_color_components;
	JSAMPLE * linear = malloc(sizeof(JSAMPLE)*readLines* dinfo.output_width * cc); // linear struct
	JSAMPLE* iter = linear;
	JSAMPROW cols[cc];
	JSAMPARRAY data[cc];
	for( int c = 0; c != cc; ++c )
	{
    		cols[ c ]=malloc( readLines * sizeof(JSAMPLE *) );

    		for( int i = 0; i != lines; ++i, iter += dinfo.output_width )
    		{
        		cols[ c ][ i ] = iter;
    		}
	data[c] = &cols[c][0];
	}	 	
	
	for( int row = 0; row < (int) dinfo.output_height; row += readLines )
	{
    		jpeg_read_raw_data( &dinfo, data, readLines );
		jpeg_write_raw_data(&cinfo, data, readLines );

	}
	
	for( int c = 0; c != cc; ++c )
	{
    		free(cols[ c ]);
	}
	free(linear);
*/
	//jvirt_barray_ptr * data =jpeg_read_coefficients(&dinfo);
	//if(data == NULL)
	//printf ("wtf");

	jpeg_write_coefficients(&cinfo,data);
	//printf ("pass3");
}



int ju_processFrame(unsigned char* inJpg, unsigned long inJpgSize, unsigned char** outJpg, unsigned long* outJpgSize)
{
	prepareDecoding(inJpg,inJpgSize);
	prepareEncoding(outJpg,outJpgSize);
	
	copyData();

	finishEncoding();// should be first to copy coefficients data
	//printf ("pass4");
	finishDecoding();
	//printf ("pass5");
	return 0;//TODO error handling!!!
}

#define MARKER_START ((unsigned char)0xFF)
#define MARKER_EOI ((unsigned char)0xD9)
#define MARKER_SOS ((unsigned char)0xDA)

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

void ju_cleanup()
{
	jpeg_destroy_decompress(&dinfo);
	jpeg_destroy_compress(&cinfo);
}
