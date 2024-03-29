/*
* Alicia Devasia
* CPSC 4040: Final Project: Cartoon Filter
* 12/3/2019
*/

#include <OpenImageIO/imageio.h>
#include <iostream>
#include <fstream>
#include <GL/glut.h>
#include<cmath>
#include <math.h>

using namespace std;
OIIO_NAMESPACE_USING


struct Pixel{ // defines a pixel structure
	unsigned char r,g,b,a;
};

//
// Global variables and constants
//
const int DEFAULTWIDTH = 600;	// default window dimensions if no image
const int DEFAULTHEIGHT = 600;

int WinWidth, WinHeight;	// window width and height
int ImWidth, ImHeight;		// image width and height
int ImChannels;           // number of channels per image pixel

int VpWidth, VpHeight;		// viewport width and height
int Xoffset, Yoffset;     // viewport offset from lower left corner of window

Pixel **pixmap = NULL;  	// the image pixmap used for OpenGL
Pixel **pixmap_SH = NULL;
Pixel **pixmap_SV = NULL;
Pixel **sobel_pixmap = NULL;
Pixel **new_intense = NULL;

int pixformat; 			     // the pixel format used to correctly  draw the image

string img_name = "";
string out_name = "";
string filter_name = "";
const int kernel_size = 3;
double kernel[kernel_size][kernel_size];
double range[kernel_size][kernel_size];
double domain[kernel_size][kernel_size];
float** intense_matrix;
float** temp_matrix;

double bottom = 0.5;

double SOBEL_V [3][3] = {{-1,0,1},
                         {-2,0,2},
                         {-1,0,1}};

double SOBEL_H [3][3] = {{1,2,1},
                         {0,0,0},
                         {-1,-2,-1}};

// Sets the value of the pixel to an int 0 or 255 if it goes below 0 or higher
// than 255
int clamp (int lo, int hi, int x) {
  return std::max(lo, std::min(x, hi));
}

// Sets the value of the pixel to an double 0 or 255 if it goes below 0 or
// higher than 255
double clamp (double lo, double hi, double x) {
  return std::max(lo, std::min(x, hi));
}

/**********************Bilateral filter******************************/

//Compute the intensity image
void intensityImg(){
	//Allocate memory for the intense_matrix
	intense_matrix = new float*[ImHeight];
  if(intense_matrix != NULL)
	intense_matrix[0] = new float[ImWidth * ImHeight];
  for(int i = 1; i < ImHeight; i++)
	intense_matrix[i] = intense_matrix[i - 1] + ImWidth;

	//Allocate memory for the temp_matrix
	temp_matrix = new float*[ImHeight];
  if(temp_matrix != NULL)
	temp_matrix[0] = new float[ImWidth * ImHeight];
  for(int i = 1; i < ImHeight; i++)
	temp_matrix[i] = temp_matrix[i - 1] + ImWidth;

	//Calculates the domain kernel
	for(int row=0; row < 3; row++){
     for(int col=0; col < 3; col++){
			 double top = pow(row-1,2)+pow(col-1,2);
			 double sigma = 2;
			 top = top/sigma;
			 domain[row][col]= exp(-top);
		 }
  }

 //Determines the intensity values for each pixel of the image
 for(int row=0; row < ImHeight; row++){
    for(int col=0; col < ImWidth; col++){
   		intense_matrix[row][col] = (pixmap[row][col].r*0.4) + (pixmap[row][col].g*0.7) + (pixmap[row][col].b*0.01);
    }
  }
}

//get the values for each element of the range kernel
void getRange(int i, int j){
	for(int x=-1; x<= 1; x++){
		for(int y=-1; y<= 1; y++){
			double rangeValue = 0.0;
			//Ensures that the range is within the range
			if(i+x >= 0 && i+x < ImHeight && j+y >= 0 && j+y < ImWidth){

				double top = intense_matrix[i+x][j+y] - intense_matrix[i][j];

				top = pow(top,2);

				top = top/98; //98 = 2(7)^2, 7 is the standard deviation
				rangeValue = exp(-top);
			}
			range[x+1][y+1]= rangeValue;

		}
	}
}


//Mulitply the domain kernel and range kernel
void domainRange(){
  for(int row=0; row < kernel_size; row++){
    for(int col=0; col < kernel_size; col++){
        kernel[row][col] = domain[row][col]*range[row][col];
    }
  }
}


// Apply the final kernel to each pixel of the intense pixmap
void apply_Intensekernel(int y, int x) {
  int center = kernel_size/2;
  double s = 0.0;

  // The kernel is centered at k_center
  // This loop takes values -1, 0, 1
  for (int j = -center; j <= center; j++) {
    for (int i = -center; i <= center; i++) {
			double modifier = kernel[center+i][center+j];
			if(i+x >= 0 && i+x < ImWidth && j+y >= 0 && j+y < ImHeight){
					s = intense_matrix[y+j][x+i] * modifier;
			}
    }
  }
	temp_matrix[y][x] = s;

}



//loop through the intense image
void apply_bilateral(){
	double r,g,b;
	r = g = b= 0.0;

	for(int i=0; i < ImHeight; i++){
		for(int j=0; j < ImWidth; j++){
			//Get the range
			//how to get the intensity values?
			getRange(i, j);

			//Mulitply domain and range
			domainRange();

			//Apply final kernel to the intensity img to create a new intesnity image
			apply_Intensekernel(i,j);


			float newr = ((float)pixmap[i][j].r/intense_matrix[i][j]*temp_matrix[i][j]);
			float newg = ((float)pixmap[i][j].g/intense_matrix[i][j]*temp_matrix[i][j]);
			float newb = ((float)pixmap[i][j].b/intense_matrix[i][j]*temp_matrix[i][j]);

			//Ensures that the new pixel values are within range
			pixmap[i][j].r = (unsigned char)clamp(0.0,255.0,newr);
			pixmap[i][j].g = (unsigned char)clamp(0.0,255.0,newg);
			pixmap[i][j].b = (unsigned char)clamp(0.0,255.0,newb);


		}
	}

}
/**********************End of Bilateral******************************/

/**********************Sobel Operator******************************/
void print(){
  cout << "SOBEL H" << endl;
      for(int row=0; row < kernel_size; row++){
        for(int col=0; col < kernel_size; col++){
          cout << SOBEL_H[row][col] << " " ;
      }
      cout << endl;
    }
  cout << endl << "SOBEL V"  << endl;
    for(int row=0; row < kernel_size; row++){
      for(int col=0; col < kernel_size; col++){

      cout << SOBEL_V[row][col] << " " ;
      }
      cout<< endl;
    }
}

// Flips the kernel horizontally and vertically
void flip_kernel(double kernelTemp[][3]){
	double temp;
	//horizontal filp
	for (int y = 0; y < kernel_size; y++) {
		for (int x = 0; x < kernel_size/2; x++) {
			temp = kernelTemp[kernel_size-x-1][y];
			kernelTemp[kernel_size-x-1][y] = kernelTemp[x][y];
			kernelTemp[x][y] = temp;
		}
	}

	temp = 0.0;
	//vertical filp
	for (int x = 0; x < kernel_size; x++) {
    for (int y = 0; y < kernel_size/2; y++) {
			temp = kernelTemp[x][kernel_size-y-1];
			kernelTemp[x][kernel_size-y-1] = kernelTemp[x][y];
			kernelTemp[x][y] = temp;
    }
  }

}

// Filps the SOBEL_V and SOBEL_H filters
// and new values stores into two kernels
void setKernel(double matrixTemp[][3]){
	//cout << "inside setKernel" << endl;
	double sum = 0.0; //calculates the scale factor
  int weight = 0;

  for(int row=0; row < kernel_size; row++){
    for(int col=0; col < kernel_size; col++){
      weight = matrixTemp[row][col];
      if(weight > 0){
        sum += weight;
      }
    }
  }

	flip_kernel(matrixTemp);

	//Apply the scale factor to the kernel
	for(int row=0; row < kernel_size; row++){
		for(int col=0; col < kernel_size; col++){
			matrixTemp[row][col] = .25*(matrixTemp[row][col]/sum);
		}
	}

  //print();
}

// Apply the kernel to each pixel of the pixmap
void apply_kernel(Pixel** pix, Pixel** temp_pixel, double matrixTemp[][3], int y, int x) {
  int center = kernel_size/2;
  double r, g, b, a;
  r = g = b = a= 0.0;

  // The kernel is centered at k_center
  // This loop takes values -1, 0, 1
  for (int j = -center; j <= center; j++) {
    for (int i = -center; i <= center; i++) {
      Pixel p = pix[y+i][x+i];
      double modifier = matrixTemp[center+i][center+j];
      r += p.r * modifier;
      g += p.g * modifier;
      b += p.b * modifier;
    }
  }

  // Make sure within RBG range of pixel
  temp_pixel[y][x].r = (int)clamp(0.0,255.0,r);
  temp_pixel[y][x].g = (int)clamp(0.0,255.0,g);
  temp_pixel[y][x].b = (int)clamp(0.0,255.0,b);
}

// Modifies the pixmap by creating temp pixmap
// and apply the kernel to the pixmap

void convolve_img(Pixel** pixmapTemp, double matrixTemp[][3]){

	int center = kernel_size/2;
	Pixel **copy_pixmap;

	// allocate space for the copy_pixmap (contiguous approach)
  copy_pixmap = new Pixel*[ImHeight];
   if(copy_pixmap != NULL)
 	copy_pixmap[0] = new Pixel[ImWidth * ImHeight];
   for(int i = 1; i < ImHeight; i++)
 	copy_pixmap[i] = copy_pixmap[i - 1] + ImWidth;

  // Apply kernel to each pixel
  // Omit outer edges
  for (int y = center; y < ImHeight-center; y++) {
    for (int x = center; x < ImWidth-center; x++) {
			apply_kernel(pixmapTemp, copy_pixmap, matrixTemp, y, x);
    }
  }
	pixmapTemp = copy_pixmap;
//	pixmap = copy_pixmap;
}

double calculateSobel(double colorH, double colorV){
   double x = pow(colorH,2);
   double y = pow(colorV,2);
   return sqrt(y+x);
}

void combineImg(){
  for(int row=0; row < ImHeight; row++){
    for(int col=0; col < ImWidth; col++){
      sobel_pixmap[row][col].r = calculateSobel(pixmap_SH[row][col].r, pixmap_SV[row][col].r);
      sobel_pixmap[row][col].g = calculateSobel(pixmap_SH[row][col].g, pixmap_SV[row][col].g);
      sobel_pixmap[row][col].b = calculateSobel(pixmap_SH[row][col].b, pixmap_SV[row][col].b);
    }
  }
}

/**********************End of Sobel******************************/

void combineBilateralSobel(){
	for(int i=0; i < ImHeight; i++){
		for(int j=0; j < ImWidth; j++){
			pixmap[i][j].r=  pixmap[i][j].r+sobel_pixmap[i][j].r;
			pixmap[i][j].g=  pixmap[i][j].g+sobel_pixmap[i][j].g;
			pixmap[i][j].b=  pixmap[i][j].b+sobel_pixmap[i][j].b;
		}
	}
}

//
//  Routine to cleanup the memory. Note that GLUT and OpenImageIO v1.6 lead to memory leaks that cannot be suppressed.
//
void destroy(){
 if (pixmap){
     delete pixmap[0];
	 delete pixmap;
  }
}

//
//  Routine to read an image file and store in a pixmap
//  returns the size of the image in pixels if correctly read, or 0 if failure
//
int readimage(string infilename){
  // Create the oiio file handler for the image, and open the file for reading the image.
  // Once open, the file spec will indicate the width, height and number of channels.
  ImageInput *infile = ImageInput::open(infilename);
  if(!infile){
    cerr << "Could not input image file " << infilename << ", error = " << geterror() << endl;
    return 0;
  }

  // Record image width, height and number of channels in global variables
  ImWidth = infile->spec().width;
  ImHeight = infile->spec().height;
  ImChannels = infile->spec().nchannels;


  // allocate temporary structure to read the image
  unsigned char tmp_pixels[ImWidth * ImHeight * ImChannels];

  // read the image into the tmp_pixels from the input file, flipping it upside down using negative y-stride,
  // since OpenGL pixmaps have the bottom scanline first, and
  // oiio expects the top scanline first in the image file.
  int scanlinesize = ImWidth * ImChannels * sizeof(unsigned char);
  if(!infile->read_image(TypeDesc::UINT8, &tmp_pixels[0] + (ImHeight - 1) * scanlinesize, AutoStride, -scanlinesize)){
    cerr << "Could not read image from " << infilename << ", error = " << geterror() << endl;
    ImageInput::destroy(infile);
    return 0;
  }

 // get rid of the old OpenGL pixmap and make a new one of the new size
  destroy();

 // allocate space for the Pixmap (contiguous approach)
  pixmap = new Pixel*[ImHeight];
  if(pixmap != NULL)
	pixmap[0] = new Pixel[ImWidth * ImHeight];
  for(int i = 1; i < ImHeight; i++)
	pixmap[i] = pixmap[i - 1] + ImWidth;

 //  assign the read pixels to the Pixmap
 int index;
  for(int row = 0; row < ImHeight; ++row) {
    for(int col = 0; col < ImWidth; ++col) {
		index = (row*ImWidth+col)*ImChannels;

		if (ImChannels==1){
			pixmap[row][col].r = tmp_pixels[index];
			pixmap[row][col].g = tmp_pixels[index];
			pixmap[row][col].b = tmp_pixels[index];
			pixmap[row][col].a = 255;
		}
		else{
			pixmap[row][col].r = tmp_pixels[index];
			pixmap[row][col].g = tmp_pixels[index+1];
			pixmap[row][col].b = tmp_pixels[index+2];
			if (ImChannels <4) // no alpha value is present so set it to 255
				pixmap[row][col].a = 255;
			else // read the alpha value
				pixmap[row][col].a = tmp_pixels[index+3];
		}
    }
  }

  // close the image file after reading, and free up space for the oiio file handler
  infile->close();
  ImageInput::destroy(infile);

  // set the pixel format to GL_RGBA and fix the # channels to 4
  pixformat = GL_RGBA;
  ImChannels = 4;

// allocate space for the Pixmap (contiguous approach)
  pixmap_SH = new Pixel*[ImHeight];
  if( pixmap_SH  != NULL)
  pixmap_SH [0] = new Pixel[ImWidth * ImHeight];
  for(int i = 1; i < ImHeight; i++)
  pixmap_SH[i] =  pixmap_SH[i - 1] + ImWidth;


  // allocate space for the Pixmap (contiguous approach)
  pixmap_SV = new Pixel*[ImHeight];
  if( pixmap_SV  != NULL)
  pixmap_SV [0] = new Pixel[ImWidth * ImHeight];
  for(int i = 1; i < ImHeight; i++)
  pixmap_SV[i] =  pixmap_SV[i - 1] + ImWidth;


    // allocate space for the Pixmap (contiguous approach)
  sobel_pixmap = new Pixel*[ImHeight];
  if( sobel_pixmap   != NULL)
  sobel_pixmap  [0] = new Pixel[ImWidth * ImHeight];
  for(int i = 1; i < ImHeight; i++)
  sobel_pixmap [i] =  sobel_pixmap [i - 1] + ImWidth;

  pixmap_SH = pixmap;
  pixmap_SV = pixmap;
 // sobel_pixmap = pixmap;
  // return image size in pixels
  return ImWidth * ImHeight;
}

//
// Routine to display a pixmap in the current window
//
void displayimage(){
  // if the window is smaller than the image, scale it down, otherwise do not scale
  if(WinWidth < ImWidth  || WinHeight < ImHeight)
    glPixelZoom(float(VpWidth) / ImWidth, float(VpHeight) / ImHeight);
  else
    glPixelZoom(1.0, 1.0);

  // display starting at the lower lefthand corner of the viewport
  glRasterPos2i(0, 0);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glDrawPixels(ImWidth, ImHeight, pixformat, GL_UNSIGNED_BYTE, pixmap[0]);
}

//
// Routine to write the current framebuffer to an image file
//
void writeimage(string outfilename){
  // make a pixmap that is the size of the window and grab OpenGL framebuffer into it
   unsigned char local_pixmap[WinWidth * WinHeight * ImChannels];
   glReadPixels(0, 0, WinWidth, WinHeight, pixformat, GL_UNSIGNED_BYTE, local_pixmap);

  // create the oiio file handler for the image
  ImageOutput *outfile = ImageOutput::create(outfilename);
  if(!outfile){
    cerr << "Could not create output image for " << outfilename << ", error = " << geterror() << endl;
    return;
  }

  // Open a file for writing the image. The file header will indicate an image of
  // width WinWidth, height WinHeight, and ImChannels channels per pixel.
  // All channels will be of type unsigned char
  ImageSpec spec(WinWidth, WinHeight, ImChannels, TypeDesc::UINT8);
  if(!outfile->open(outfilename, spec)){
    cerr << "Could not open " << outfilename << ", error = " << geterror() << endl;
    ImageOutput::destroy(outfile);
    return;
  }

  // Write the image to the file. All channel values in the pixmap are taken to be
  // unsigned chars. While writing, flip the image upside down by using negative y stride,
  // since OpenGL pixmaps have the bottom scanline first, and oiio writes the top scanline first in the image file.
  int scanlinesize = WinWidth * ImChannels * sizeof(unsigned char);
  if(!outfile->write_image(TypeDesc::UINT8, local_pixmap + (WinHeight - 1) * scanlinesize, AutoStride, -scanlinesize)){
    cerr << "Could not write image to " << outfilename << ", error = " << geterror() << endl;
    ImageOutput::destroy(outfile);
    return;
  }

  // close the image file after the image is written and free up space for the
  // ooio file handler
  outfile->close();
  ImageOutput::destroy(outfile);
}

//
//   Display Callback Routine: clear the screen and draw the current image
//
void handleDisplay(){

  // specify window clear (background) color to be opaque black
  glClearColor(0, 0, 0, 1);
  // clear window to background color
  glClear(GL_COLOR_BUFFER_BIT);

  // only draw the image if it is of a valid size
  if(ImWidth > 0 && ImHeight > 0)
    	displayimage();

  // flush the OpenGL pipeline to the viewport
  glFlush();
}


//
//  Keyboard Callback Routine: 'r' - read and display a new image,
//  'w' - write the current window to an image file, 'q' or ESC - quit
//
void handleKey(unsigned char key, int x, int y) {
	int ok;

	switch (key) {
	case 's':		// 's' - apply the sobel operator to the image
		convolve_img(pixmap_SH, SOBEL_H); //make pixmapSH
		convolve_img(pixmap_SV, SOBEL_V); //make pixmapSV
		combineImg(); //combine the two pixmap
		combineBilateralSobel();
  	glutReshapeWindow(WinWidth, WinHeight); // OpenGL window should match new image
		glutPostRedisplay();
		break;
  case 'b': // 'b' - apply the bilateral filter to the image
		apply_bilateral();
    glutReshapeWindow(WinWidth, WinHeight); // OpenGL window should match new image
		glutPostRedisplay();
		break;
	case 'w':		// 'w' - write the current image to a file
		writeimage(out_name);
		break;

	case 'q':		// q or ESC - quit
	case 'Q':
	case 27:
		destroy();
		exit(0);

	default:		// not a valid key -- just ignore it
		return;
	}
}

//
//  Reshape Callback Routine: If the window is too small to fit the image,
//  make a viewport of the maximum size that maintains the image proportions.
//  Otherwise, size the viewport to match the image size. In either case, the
//  viewport is centered in the window.
//
void handleReshape(int w, int h){
  float imageaspect = (float)ImWidth / (float)ImHeight;	// aspect ratio of image
  float newaspect = (float)w / (float)h; // new aspect ratio of window

  // record the new window size in global variables for easy access
  WinWidth = w;
  WinHeight = h;

  // if the image fits in the window, viewport is the same size as the image
  if(w >= ImWidth && h >= ImHeight){
    Xoffset = (w - ImWidth) / 2;
    Yoffset = (h - ImHeight) / 2;
    VpWidth = ImWidth;
    VpHeight = ImHeight;
  }
  // if the window is wider than the image, use the full window height
  // and size the width to match the image aspect ratio
  else if(newaspect > imageaspect){
    VpHeight = h;
    VpWidth = int(imageaspect * VpHeight);
    Xoffset = int((w - VpWidth) / 2);
    Yoffset = 0;
  }
  // if the window is narrower than the image, use the full window width
  // and size the height to match the image aspect ratio
  else{
    VpWidth = w;
    VpHeight = int(VpWidth / imageaspect);
    Yoffset = int((h - VpHeight) / 2);
    Xoffset = 0;
  }

  // center the viewport in the window
  glViewport(Xoffset, Yoffset, VpWidth, VpHeight);

  // viewport coordinates are simply pixel coordinates
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, VpWidth, 0, VpHeight);
  glMatrixMode(GL_MODELVIEW);
}

//
// Main program to scan the commandline, set up GLUT and OpenGL, and start Main Loop
//
int main(int argc, char* argv[]) {
	// scan command line and process
	// only one parameter allowed, an optional image filename and extension
	//Check to see the user entered four command arugments
	if (argc > 3) {
		cerr << "USAGE: ./cartoon <in.png> <out.png>" << endl;
		return 1;
	}

	// set up the default window and empty pixmap if no image or image fails to load
	WinWidth = DEFAULTWIDTH;
	WinHeight = DEFAULTHEIGHT;
	ImWidth = 0;
	ImHeight = 0;

	img_name = argv[1];
	out_name = argv[2];

	// load the image if present, and size the window to match
	if (argc == 3) {
		if (readimage(img_name)) {

		}
	}
	setKernel(SOBEL_V);
	setKernel(SOBEL_H);

	intensityImg();

	// start up GLUT
	glutInit(&argc, argv);

	// create the graphics window, giving width, height, and title text
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowSize(WinWidth, WinHeight);
	glutCreateWindow("Cartoon");

	// set up the callback routines
	glutDisplayFunc(handleDisplay); // display update callback
	glutKeyboardFunc(handleKey);	  // keyboard key press callback
	glutReshapeFunc(handleReshape); // window resize callback


	// Enter GLUT's event loop
	glutMainLoop();
	return 0;
}
