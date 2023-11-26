/// image8bit - A simple image processing module.
///
/// This module is part of a programming project
/// for the course AED, DETI / UA.PT
///
/// You may freely use and modify this code, at your own risk,
/// as long as you give proper credit to the original and subsequent authors.
///
/// João Manuel Rodrigues <jmr@ua.pt>
/// 2013, 2023

// Student authors (fill in below):
// NMec: 113626  Name: Rodrigo Abreu
// NMec: 113482  Name: João Neto
// 
// 
// 
// Date: 16/11/2023
//

#define MAX(X,Y) (((X)>(Y)) ? (X) : (Y))
#define MIN(X,Y) (((X)<(Y)) ? (X) : (Y))

#include "image8bit.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "instrumentation.h"

// The data structure
//
// An image is stored in a structure containing 3 fields:
// Two integers store the image width and height.
// The other field is a pointer to an array that stores the 8-bit gray
// level of each pixel in the image.  The pixel array is one-dimensional
// and corresponds to a "raster scan" of the image from left to right,
// top to bottom.
// For example, in a 100-pixel wide image (img->width == 100),
//   pixel position (x,y) = (33,0) is stored in img->pixel[33];
//   pixel position (x,y) = (22,1) is stored in img->pixel[122].
// 
// Clients should use images only through variables of type Image,
// which are pointers to the image structure, and should not access the
// structure fields directly.

// Maximum value you can store in a pixel (maximum maxval accepted)
const uint8 PixMax = 255;

// Internal structure for storing 8-bit graymap images
struct image {
  int width;
  int height;
  int maxval;   // maximum gray value (pixels with maxval are pure WHITE)
  uint8* pixel; // pixel data (a raster scan)
};


// This module follows "design-by-contract" principles.
// Read `Design-by-Contract.md` for more details.

/// Error handling functions

// In this module, only functions dealing with memory allocation or file
// (I/O) operations use defensive techniques.
// 
// When one of these functions fails, it signals this by returning an error
// value such as NULL or 0 (see function documentation), and sets an internal
// variable (errCause) to a string indicating the failure cause.
// The errno global variable thoroughly used in the standard library is
// carefully preserved and propagated, and clients can use it together with
// the ImageErrMsg() function to produce informative error messages.
// The use of the GNU standard library error() function is recommended for
// this purpose.
//
// Additional information:  man 3 errno;  man 3 error;

// Variable to preserve errno temporarily
static int errsave = 0;

// Error cause
static char* errCause;

/// Error cause.
/// After some other module function fails (and returns an error code),
/// calling this function retrieves an appropriate message describing the
/// failure cause.  This may be used together with global variable errno
/// to produce informative error messages (using error(), for instance).
///
/// After a successful operation, the result is not garanteed (it might be
/// the previous error cause).  It is not meant to be used in that situation!
char* ImageErrMsg() { ///
  return errCause;
}


// Defensive programming aids
//
// Proper defensive programming in C, which lacks an exception mechanism,
// generally leads to possibly long chains of function calls, error checking,
// cleanup code, and return statements:
//   if ( funA(x) == errorA ) { return errorX; }
//   if ( funB(x) == errorB ) { cleanupForA(); return errorY; }
//   if ( funC(x) == errorC ) { cleanupForB(); cleanupForA(); return errorZ; }
//
// Understanding such chains is difficult, and writing them is boring, messy
// and error-prone.  Programmers tend to overlook the intricate details,
// and end up producing unsafe and sometimes incorrect programs.
//
// In this module, we try to deal with these chains using a somewhat
// unorthodox technique.  It resorts to a very simple internal function
// (check) that is used to wrap the function calls and error tests, and chain
// them into a long Boolean expression that reflects the success of the entire
// operation:
//   success = 
//   check( funA(x) != error , "MsgFailA" ) &&
//   check( funB(x) != error , "MsgFailB" ) &&
//   check( funC(x) != error , "MsgFailC" ) ;
//   if (!success) {
//     conditionalCleanupCode();
//   }
//   return success;
// 
// When a function fails, the chain is interrupted, thanks to the
// short-circuit && operator, and execution jumps to the cleanup code.
// Meanwhile, check() set errCause to an appropriate message.
// 
// This technique has some legibility issues and is not always applicable,
// but it is quite concise, and concentrates cleanup code in a single place.
// 
// See example utilization in ImageLoad and ImageSave.
//
// (You are not required to use this in your code!)


// Check a condition and set errCause to failmsg in case of failure.
// This may be used to chain a sequence of operations and verify its success.
// Propagates the condition.
// Preserves global errno!
static int check(int condition, const char* failmsg) {
  errCause = (char*)(condition ? "" : failmsg);
  return condition;
}


/// Init Image library.  (Call once!)
/// Currently, simply calibrate instrumentation and set names of counters.
void ImageInit(void) { ///
  InstrCalibrate();
  InstrName[0] = "pixmem";  // InstrCount[0] will count pixel array acesses
  // Name other counters here...
  InstrName[1]= "pixcomp";
  InstrName[2]= "iterações";
}

// Macros to simplify accessing instrumentation counters:
#define PIXMEM InstrCount[0]
#define COMPARACOES InstrCount[1] //macro para analise de complexidade ImageLocateSubImage
#define ITER InstrCount[2]
// Add more macros here...

// TIP: Search for PIXMEM or InstrCount to see where it is incremented!


/// Image management functions

/// Create a new black image.
///   width, height : the dimensions of the new image.
///   maxval: the maximum gray level (corresponding to white).
/// Requires: width and height must be non-negative, maxval > 0.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageCreate(int width, int height, uint8 maxval) { 
  assert (width >= 0);
  assert (height >= 0);
  assert (0 < maxval && maxval <= PixMax);
  
  Image img = malloc(sizeof(struct image)); //aloca memoria para a estrutura

  if (check(img != NULL, "Failed to allocate memory for image")){ //verifica se a memoria foi alocada
    img->width = width; //atribui os valores aos campos da estrutura
    img->height = height;
    img->maxval = maxval;
    img->pixel = malloc(sizeof(uint8)*height*width); //aloca memoria para o array de pixeis

    if (check(img->pixel != NULL, "Failed to allocate memory for image pixels")){ //verifica se a memoria foi alocada
      return img;
    }
    else {
      free(img->pixel); //liberta a memoria alocada para o array de pixeis
      free(img); 
      return NULL;
    }
  }
  else{
    free(img); //liberta a memoria alocada para a estrutura
    return NULL;
  }
}

// Declaração de ponteiros para as tabelas de soma
int *sumtable1;
int *sumtable2;
int *sumtableQ1;
int *sumtableQ2;

static inline int G(Image img, int x, int y);

// Function to calculate sumtables
void calculateSumTables_Quadrado(Image img1, Image img2) {
   // Verifica se as imagens não são nulas
  assert (img1 != NULL);
  assert (img2 != NULL);

  // Aloca memória para a tabela de soma Q2
  sumtableQ2 = (int*) malloc(sizeof(int) * img2->width * img2->height);
  // Aloca memória para a tabela de soma Q1
  sumtableQ1 = (int*) malloc(sizeof(int) * img1->width * img1->height);

  // Preenche a tabela de soma Q2
  for (int x=0; x<img2->width; x++){
    for (int y=0; y<img2->height; y++){
      ITER++;
      sumtableQ2[G(img2,x,y)]=ImageGetPixel(img2, x, y)*ImageGetPixel(img2, x, y);//o valor de cada elemento da tabela das somas é igual ao valor de cinzento ao quadrado do pixel (x,y) correspondente
      if (x> 0 && y>0){                                                           //+ o valor do elemento ao quadrado (x-1,y) + o valor do elemento ao quadrado (x,y-1) - o valor do elemento ao quadrado (x-1,y-1).
        sumtableQ2[G(img2,x,y)]-=sumtableQ2[G(img2,x-1,y-1)];                     //Para mais informações ver vídeo https://youtu.be/4Eh0y3LHTNU?si=1JzPZSPd9p0rDr-U
      }
      if (x > 0){
        sumtableQ2[G(img2,x,y)]+=sumtableQ2[G(img2,x-1,y)];
      }
      if (y > 0){
        sumtableQ2[G(img2,x,y)]+=sumtableQ2[G(img2,x,y-1)];
      }
      //As condições anteriores são necessárias para evitar que sejam somados pixeis que não pertencem à imagem
    }
  }
  // Preenche a tabela de soma Q1
  for (int x=0; x<img1->width; x++){
    for (int y=0; y<img1->height; y++){
      ITER++;
      sumtableQ1[G(img1,x,y)]=ImageGetPixel(img1, x, y)*ImageGetPixel(img1, x, y);//igual ao calculo da soma Q2
      if (x> 0 && y>0){
        sumtableQ1[G(img1,x,y)]-=sumtableQ1[G(img1,x-1,y-1)];
      }
      if (x > 0){
        sumtableQ1[G(img1,x,y)]+=sumtableQ1[G(img1,x-1,y)];
      }
      if (y > 0){
        sumtableQ1[G(img1,x,y)]+=sumtableQ1[G(img1,x,y-1)];
      }
    }
  }
}
// Libera memória alocada para as tabelas de soma Q1 e Q2
void freeSumTables_Quadrado() {
    free(sumtableQ1);
    sumtableQ1=NULL;
    free(sumtableQ2);
    sumtableQ2=NULL;
}

// Função para calcular tabelas de soma
void calculateSumTables(Image img1, Image img2) {
  // Verifica se as imagens não são nulas
  assert (img1 != NULL);
  assert (img2 != NULL);

  // Aloca memória para a tabela de soma 2
  sumtable2 = (int*) malloc(sizeof(int) * img2->width * img2->height);
  // Aloca memória para a tabela de soma 1
  sumtable1 = (int*) malloc(sizeof(int) * img1->width * img1->height);
  // Preenche a tabela de soma 2
  for (int x=0; x<img2->width; x++){
    for (int y=0; y<img2->height; y++){
      ITER++;
      sumtable2[G(img2,x,y)]=ImageGetPixel(img2, x, y);     //o valor de cada elemento da tabela das somas é igual ao valor de cinzento do pixel (x,y) correspondente
      if (x> 0 && y>0){                                     //+ o valor do elemento ao quadrado (x-1,y) + o valor do elemento ao quadrado (x,y-1) - o valor do elemento ao quadrado (x-1,y-1).
        sumtable2[G(img2,x,y)]-=sumtable2[G(img2,x-1,y-1)]; //Para mais informações ver vídeo https://youtu.be/4Eh0y3LHTNU?si=1JzPZSPd9p0rDr-U
      }
      if (x > 0){
        sumtable2[G(img2,x,y)]+=sumtable2[G(img2,x-1,y)];
      }
      if (y > 0){
        sumtable2[G(img2,x,y)]+=sumtable2[G(img2,x,y-1)];
      }
      //As condições anteriores são necessárias para evitar que sejam somados pixeis que não pertencem à imagem
    }
  }
  // Preenche a tabela de soma 1
  for (int x=0; x<img1->width; x++){
    for (int y=0; y<img1->height; y++){
      ITER++;
      sumtable1[G(img1,x,y)]=ImageGetPixel(img1, x, y);   //igual ao calculo da soma 2
      if (x> 0 && y>0){
        sumtable1[G(img1,x,y)]-=sumtable1[G(img1,x-1,y-1)];
      }
      if (x > 0){
        sumtable1[G(img1,x,y)]+=sumtable1[G(img1,x-1,y)];
      }
      if (y > 0){
        sumtable1[G(img1,x,y)]+=sumtable1[G(img1,x,y-1)];
      }
    }
  }
}
// Libera memória alocada para as tabelas de soma 1 e 2
void freeSumTables() {
  free(sumtable1);
  sumtable1 = NULL;
  free(sumtable2);
  sumtable2 = NULL;
}

/// Destroy the image pointed to by (*imgp).
///   imgp : address of an Image variablestatic inline int G(Image img, int x, int y);.
/// If (*imgp)==NULL, no operation is performed.
/// Ensures: (*imgp)==NULL.
/// Should never fail, and should preserve global errno/errCause.
void ImageDestroy(Image* imgp) { ///
  assert (imgp != NULL);
  free((*imgp)->pixel); //liberta a memoria alocada para o array de pixeis
  free(*imgp); //liberta a memoria alocada para a estrutura
  *imgp = NULL;  
}


/// PGM file operations

// See also:
// PGM format specification: http://netpbm.sourceforge.net/doc/pgm.html

// Match and skip 0 or more comment lines in file f.
// Comments start with a # and continue until the end-of-line, inclusive.
// Returns the number of comments skipped.
static int skipComments(FILE* f) {
  char c;
  int i = 0;
  while (fscanf(f, "#%*[^\n]%c", &c) == 1 && c == '\n') {
    i++;
  }
  return i;
}

/// Load a raw PGM file.
/// Only 8 bit PGM files are accepted.
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageLoad(const char* filename) { ///
  int w, h;
  int maxval;
  char c;
  FILE* f = NULL;
  Image img = NULL;

  int success = 
  check( (f = fopen(filename, "rb")) != NULL, "Open failed" ) &&
  // Parse PGM header
  check( fscanf(f, "P%c ", &c) == 1 && c == '5' , "Invalid file format" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d ", &w) == 1 && w >= 0 , "Invalid width" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d ", &h) == 1 && h >= 0 , "Invalid height" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d", &maxval) == 1 && 0 < maxval && maxval <= (int)PixMax , "Invalid maxval" ) &&
  check( fscanf(f, "%c", &c) == 1 && isspace(c) , "Whitespace expected" ) &&
  // Allocate image
  (img = ImageCreate(w, h, (uint8)maxval)) != NULL &&
  // Read pixels
  check( fread(img->pixel, sizeof(uint8), w*h, f) == w*h , "Reading pixels" );
  PIXMEM += (unsigned long)(w*h);  // count pixel memory accesses

  // Cleanup
  if (!success) {
    errsave = errno;
    ImageDestroy(&img);
    errno = errsave;
  }
  if (f != NULL) fclose(f);
  return img;
}

/// Save image to PGM file.
/// On success, returns nonzero.
/// On failure, returns 0, errno/errCause are set appropriately, and
/// a partial and invalid file may be left in the system.
int ImageSave(Image img, const char* filename) { ///
  assert (img != NULL);
  int w = img->width;
  int h = img->height;
  uint8 maxval = img->maxval;
  FILE* f = NULL;

  int success =
  check( (f = fopen(filename, "wb")) != NULL, "Open failed" ) &&
  check( fprintf(f, "P5\n%d %d\n%u\n", w, h, maxval) > 0, "Writing header failed" ) &&
  check( fwrite(img->pixel, sizeof(uint8), w*h, f) == w*h, "Writing pixels failed" ); 
  PIXMEM += (unsigned long)(w*h);  // count pixel memory accesses

  // Cleanup
  if (f != NULL) fclose(f);
  return success;
}


/// Information queries

/// These functions do not modify the image and never fail.

/// Get image width
int ImageWidth(Image img) { ///retorna a largura da imagem
  assert (img != NULL);
  return img->width; 
}

/// Get image height
int ImageHeight(Image img) { ///retorna a altura da imagem
  assert (img != NULL);
  return img->height;
}

/// Get image maximum gray level
int ImageMaxval(Image img) { ///retorna o valor maximo da intensidade de cinzento da imagem
  assert (img != NULL);
  return img->maxval;
}

/// Pixel stats
/// Find the minimum and maximum gray levels in image.
/// On return,
/// *min is set to the minimum gray level in the image,
/// *max is set to the maximum.
void ImageStats(Image img, uint8* min, uint8* max) { 
  assert (img != NULL);
  *min = PixMax; //atribui o valor maximo possivel a min
  *max = 0; //atribui o valor minimo possivel a max
  size_t area=img->width*img->height; //calcula a area da imagem, numero total de pixeis
  for (int i = 0; i < area; i++) {
    PIXMEM += 2;  // conta 2 acessos à memória
    if (img->pixel[i] < *min) {  //se o valor do pixel for menor que o valor minimo atual, o valor minimo atual passa a ser o valor do pixel
      *min = img->pixel[i];
    }                             
    if (img->pixel[i] > *max) {  //se o valor do pixel for maior que o valor maximo atual, o valor maximo atual passa a ser o valor do pixel
      *max = img->pixel[i];
    }
  }
}

/// Check if pixel position (x,y) is inside img.
int ImageValidPos(Image img, int x, int y) { ///
  assert (img != NULL);
  return (0 <= x && x < img->width) && (0 <= y && y < img->height);
}

/// Check if rectangular area (x,y,w,h) is completely inside img.
int ImageValidRect(Image img, int x, int y, int w, int h) { ///
  assert (img != NULL);
  return (0 <= x && x+w <= img->width) && (0 <= y &&y+h <= img->height); 
}

/// Pixel get & set operations

/// These are the primitive operations to access and modify a single pixel
/// in the image.
/// These are very simple, but fundamental operations, which may be used to 
/// implement more complex operations.

// Transform (x, y) coords into linear pixel index.
// This internal function is used in ImageGetPixel / ImageSetPixel. 
// The returned index must satisfy (0 <= index < img->width*img->height)
static inline int G(Image img, int x, int y) {
  int index = y*img->width + x;  //expressão para converter coordenadas (x,y) em indice linear
  assert (0 <= index && index < img->width*img->height);
  return index;
}

/// Get the pixel (level) at position (x,y).
uint8 ImageGetPixel(Image img, int x, int y) { ///retorna o valor de intensidade cinzento de um pixel
  assert (img != NULL);
  assert (ImageValidPos(img, x, y));  
  PIXMEM += 1; // count one pixel access (read)
  return img->pixel[G(img, x, y)];
} 

/// Set the pixel at position (x,y) to new level.
void ImageSetPixel(Image img, int x, int y, uint8 level) { ///Atribui um valor de intensidade cinzento a um pixel
  assert (img != NULL);
  assert (ImageValidPos(img, x, y));  
  PIXMEM += 1; // count one pixel access (store)
  img->pixel[G(img, x, y)] = level;
} 


/// Pixel transformations

/// These functions modify the pixel levels in an image, but do not change
/// pixel positions or image geometry in any way.
/// All of these functions modify the image in-place: no allocation involved.
/// They never fail.


/// Transform image to negative image.
/// This transforms dark pixels to light pixels and vice-versa,
/// resulting in a "photographic negative" effect.
void ImageNegative(Image img) { ///
  assert (img != NULL);
  size_t area=img->width*img->height; //area da imagem, numero total de pixeis
  for (int i = 0; i < area; i++) {
    PIXMEM += 2;  // conta o acesso a pixeis
    img->pixel[i] = PixMax - img->pixel[i]; //inverte o valor de intensidade cinzento de cada pixel
  }
}

/// Apply threshold to image.
/// Transform all pixels with level<thr to black (0) and
/// all pixels with level>=thr to white (maxval).
void ImageThreshold(Image img, uint8 thr) { ///
  assert (img != NULL);
  size_t area=img->width*img->height; //area da imagem, numero total de pixeis
  for (int i = 0; i < area; i++) {
    PIXMEM ++;  // conta acesso a um pixel
    if (img->pixel[i] < thr) { //condições para o caso do valor de intensidade exceder os limites
      PIXMEM ++; 
      img->pixel[i] = 0; //se o valor de intensidade for menor que o threshold, o pixel fica preto
    }
    else {
      PIXMEM ++; 
      img->pixel[i] = img->maxval; //se o valor de intensidade for maior ou igual ao threshold, o pixel fica branco
    }
  }
}

/// Brighten image by a factor.
/// Multiply each pixel level by a factor, but saturate at maxval.
/// This will brighten the image if factor>1.0 and
/// darken the image if factor<1.0.
void ImageBrighten(Image img, double factor) { ///
  assert (img != NULL);
  size_t area=img->width*img->height; //area da imagem, numero total de pixeis
  for (int i = 0; i < area; i++) {
    PIXMEM++;  // conta acesso a um pixel
    if (img->pixel[i] * factor + 0.5 > img->maxval) {
      PIXMEM++;
      img->pixel[i] = img->maxval; //se o resultado da operação ultrapassar o valor da intensidade limite, 
    }                              //o valor de intensidade do pixel fica igual ao valor da intensidade limite
    else {
      PIXMEM+=2;
      img->pixel[i] = img->pixel[i] * factor + 0.5; //multiplica o valor de intensidade cinzento de cada pixel pelo factor
    }                                               //e arredonda o resultado para o inteiro mais proximo
  }
}

/// Geometric transformations

/// These functions apply geometric transformations to an image,
/// returning a new image as a result.
/// 
/// Success and failure are treated as in ImageCreate:
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.

// Implementation hint: 
// Call ImageCreate whenever you need a new image!

/// Rotate an image.
/// Returns a rotated version of the image.
/// The rotation is 90 degrees anti-clockwise.
/// Ensures: The original img is not modified.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageRotate(Image img) { ///
  assert (img != NULL);
  // Insert your code here!
  Image rotatedImg = ImageCreate(img->height, img->width, img->maxval); // cria uma nova imagem, com os valores de largura e altura trocados
  for (int i = 0; i < img->height; i++) {                               // isto é necessário para imagens não quadradas
    for (int j = 0; j < img->width; j++) {
      ImageSetPixel(rotatedImg, i, img->width - j -1, ImageGetPixel(img, j, i)); 
    } //aplica-se a rotação de 90º anti-horario a cada pixel da imagem
  }
  return rotatedImg; //retorna a imagem
}

/// Mirror an image = flip left-right.
/// Returns a mirrored version of the image.
/// Ensures: The original img is not modified.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageMirror(Image img) { ///
  assert (img != NULL);
  // Insert your code here!
  Image mirroredImg = ImageCreate(img->width, img->height, img->maxval); //cria uma nova imagem com as mesmas dimensões da anterior
  for (int i = 0; i < img->height; i++) {
    for (int j = 0; j < img->width; j++) {
      ImageSetPixel(mirroredImg, j, i, ImageGetPixel(img, img->width - j - 1, i)); //para todos os pixeis, inverte as cordenadas do eixo do x
                                                                                   //x=width-x-1
    }
  }
  return mirroredImg; //retorna a nova imagem, com as alterações efetuadas
}

/// Crop a rectangular subimage from img.
/// The rectangle is specified by the top left corner coords (x, y) and
/// width w and height h.
/// Requires:
///   The rectangle must be inside the original image.
/// Ensures:
///   The original img is not modified.
///   The returned image has width w and height h.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageCrop(Image img, int x, int y, int w, int h) {
  assert(img != NULL);
  assert(ImageValidRect(img, x, y, w, h)); //verifica se as dimensões a serem cortadas pertencem completamente à àrea da imagem

  Image croppedImg = ImageCreate(w, h, img->maxval); //cria uma nova imagem com as novas dimensões de largura e altura
  for (int i = 0; i < h; i++) {
    for (int j = 0; j < w; j++) {
      ImageSetPixel(croppedImg, j, i, ImageGetPixel(img, x + j, y + i)); //atribui o valor de cinzento dos pixeis correspondentes à nova imagem
    }
  }

  return croppedImg; //retorna a nova imagem
}

/// Operations on two images

/// Paste an image into a larger image.
/// Paste img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
void ImagePaste(Image img1, int x, int y, Image img2) { ///
  assert (img1 != NULL);
  assert (img2 != NULL);
  assert (ImageValidRect(img1, x, y, img2->width, img2->height));

  for (int i = 0; i < img2->height; i++) {
    for (int j = 0; j < img2->width; j++) {
      ITER++;
      ImageSetPixel(img1, x + j, y + i, ImageGetPixel(img2, j, i)); //atribui o valor de cinzento dos pixeis correspondentes à nova imagem
    }
  }
}

/// Blend an image into a larger image.
/// Blend img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
/// alpha usually is in [0.0, 1.0], but values outside that interval
/// may provide interesting effects.  Over/underflows should saturate.
void ImageBlend(Image img1, int x, int y, Image img2, double alpha) { ///
  assert (img1 != NULL);
  assert (img2 != NULL);
  assert (ImageValidRect(img1, x, y, img2->width, img2->height));

  for (int i = 0; i < img2->width; i++){
    for (int j =0; j < img2->height; j++){
      ImageSetPixel(img1, x + i, y + j, (uint8)(ImageGetPixel(img1, x + i, y + j) * (1 - alpha) + 0.5 + ImageGetPixel(img2, i, j) * alpha)); 
    } 
  }
}

/// Compare an image to a subimage of a larger image.
/// Returns 1 (true) if img2 matches subimage of img1 at pos (x, y).
/// Returns 0, otherwise.
// Função para verificar se uma subimagem coincide com uma parte específica de outra imagem
int ImageMatchSubImage(Image img1, int x, int y, Image img2) {
  // Verifica se as imagens não são nulas
  assert(img1 != NULL);
  assert(img2 != NULL);

  // Calcula as coordenadas máximas da subimagem em img1
  int max_height = y + img2->height - 1;
  int max_width = x + img2->width - 1;

  // Variáveis para armazenar as somas de colunas e linhas
  int sum_cols = 0;
  int sum_rows = 0;

  // Verifica as somas das colunas
  for (int wid = 0; wid < img2->width; wid++) {
    ITER++;
    // Calcula a soma da coluna na imagem maior (img1)
    sum_cols = sumtable1[G(img1, x + wid, max_height)];
    if (x + wid > 0) {
      sum_cols -= sumtable1[G(img1, x + wid - 1, max_height)];
    }
    if (y > 0) {
      sum_cols -= sumtable1[G(img1, x + wid, y - 1)];
    }
    if (x + wid > 0 && y > 0) {
      sum_cols += sumtable1[G(img1, x + wid - 1, y - 1)];
    }

    // Compara as somas das colunas da subimagem com as colunas correspondentes da img2
    COMPARACOES++;
    if (wid == 0) {
      if (sum_cols != sumtable2[G(img2, wid, img2->height - 1)]) {
        return 0; // Retorna 0 se não houver correspondência
      }
    } else {
      if (sum_cols != sumtable2[G(img2, wid, img2->height - 1)] - sumtable2[G(img2, wid - 1, img2->height - 1)]) {
        return 0; // Retorna 0 se não houver correspondência
      }
    }
  }

  // Verifica as somas das linhas
  for (int hei = 0; hei < img2->height; hei++) {
    ITER++;
    // Calcula a soma da linha da subimagem
    sum_rows = sumtable1[G(img1, max_width, y + hei)];
    if (x > 0) {
      sum_rows -= sumtable1[G(img1, x - 1, y + hei)];
    }
    if (y + hei > 0) {
      sum_rows -= sumtable1[G(img1, max_width, y + hei - 1)];
    }
    if (x > 0 && y + hei > 0) {
      sum_rows += sumtable1[G(img1, x - 1, y + hei - 1)];
    }

    // Compara as somas das linhas da subimagem com as linhas correspondentes da img2
    COMPARACOES++;
    if (hei == 0) {
      if (sum_rows != sumtable2[G(img2, img2->width - 1, hei)]) {
        return 0; // Retorna 0 se não houver correspondência
      }
    } else {
      if (sum_rows != sumtable2[G(img2, img2->width - 1, hei)] - sumtable2[G(img2, img2->width - 1, hei - 1)]) {
        return 0; // Retorna 0 se não houver correspondência
      }
    }
  }
  // Estas comparações entre linhas e colunas permitem, excluir casos em que somente o pixel final troca com por exemplo o superior
  // Verifica se os pixeis nas posições correspondentes das duas imagens são iguais
  for (int i = 0; i < img2->height; i++) {
    for (int j = 0; j < img2->width; j++) {
      ITER++;
      COMPARACOES++;
      if (ImageGetPixel(img1, x + j, y + i) != ImageGetPixel(img2, j, i)) {
        return 0; // Retorna 0 se os pixeis não coincidirem
      }
    }
  }
  // Acabamos por ter de comparar pixel a pixel na mesma pq vai sempre existir um caso muito específico em que vamos ter de comparar pixel a pixel
  // Este conjunto de comparações tanto a nível de somas normais e ao quadrado (ImageLocateSubImage), como comparar a soma das linhas e colunas de cada imagem
  // Permitem excluir casos mais "gerais" de imagens que possam passar muito tempo a comparar-se pixel a pixel
  // Permitindo assim uma maior rapidez geral no código
  // Só casos já muito específicos é que conseguem passar pelas 4 verificações que fizemos
  return 1; // Retorna 1 se todas as verificações foram bem-sucedidas, indicando uma correspondência
}

/// Locate a subimage inside another image.
/// Searches for img2 inside img1.
/// If a match is found, returns 1 and matching position is set in vars (*px, *py).
/// If no match is found, returns 0 and (*px, *py) are left untouched.

// Função para localizar uma subimagem em outra imagem
int ImageLocateSubImage(Image img1, int* px, int* py, Image img2) {
  // Verifica se as imagens não são nulas
  assert(img1 != NULL);
  assert(img2 != NULL);

  // Chama as funções para calcular as tabelas de soma
  calculateSumTables(img1, img2);
  calculateSumTables_Quadrado(img1, img2);

  // Obtém as somas finais das tabelas da img2
  int sum2 = sumtable2[G(img2, img2->width - 1, img2->height - 1)];
  int sumQ2 = sumtableQ2[G(img2, img2->width - 1, img2->height - 1)];

  // Itera sobre a imagem maior para procurar a subimagem
  for (int i = img2->height - 1; i < img1->height; i++) {
    for (int j = img2->width - 1; j < img1->width; j++) {
      ITER++;
      // Obtém as somas locais das tabelas da img1
      int sum1 = sumtable1[G(img1, j, i)];
      int sumQ1 = sumtableQ1[G(img1, j, i)];

      // Ajusta as somas subtraindo as partes que não são necessárias
      if (j - img2->width + 1 > 0) {
        sum1 -= sumtable1[G(img1, j - img2->width, i)];
        sumQ1 -= sumtableQ1[G(img1, j - img2->width, i)];
      }
      if (i - img2->height + 1 > 0) {
        sum1 -= sumtable1[G(img1, j, i - img2->height)];
        sumQ1 -= sumtableQ1[G(img1, j, i - img2->height)];
      }
      if (j - img2->width + 1 > 0 && i - img2->height + 1 > 0) {
        sum1 += sumtable1[G(img1, j - img2->width, i - img2->height)];
        sumQ1 += sumtableQ1[G(img1, j - img2->width, i - img2->height)];
      }

      // Compara as somas para verificar se as sub-imagens correspondem
      COMPARACOES++;
      if (sum1 == sum2) {
        COMPARACOES++;
        if (sumQ1 == sumQ2) {
          // Usamos as duas sumtables (ao quadrado e normal), pois imagens com a mesma área podem ter tons de cinzento diferente, e assim garantimos que pelos menos os tons de cinzentos são todos iguais
          // Se as somas coincidirem, verifica se as sub-imagens correspondem
          if (ImageMatchSubImage(img1, j - img2->width + 1, i - img2->height + 1, img2)) {
            // Se houver correspondência, atribui as coordenadas e liberta a memória
            *px = j - img2->width + 1;
            *py = i - img2->height + 1;
            freeSumTables();
            freeSumTables_Quadrado();
            return 1; // Retorna 1 indicando correspondência encontrada
          }
        }
      }
    }
  }

  // Liberta a memória e retorna 0 se nenhuma correspondência for encontrada
  freeSumTables();
  freeSumTables_Quadrado();
  return 0;
}


/// Filtering

/// Blur an image by a applying a (2dx+1)x(2dy+1) mean filter.
/// Each pixel is substituted by the mean of the pixels in the rectangle
/// [x-dx, x+dx]x[y-dy, y+dy].
/// The image is changed in-place.

void ImageBlur(Image img, int dx, int dy){ //Versão otimizada da função ImageBlur, através da tabela das somas
  assert(img!=NULL);
  int *sumtable; //declaração da tabela de somas
  sumtable = (int*) malloc(sizeof(uint8*)*img->width *img->height); //aloca memoria para a tabela de somas, que é igual ao numero de pixeis da imagem
  for (int x=0; x< img->width; x++){ //itera-se por todos os pixeis da imagem e calcula-se a tabela de somas,
    for (int y=0; y<img->height; y++){  //dependendo do valor de cinzento de cada pixel
      ITER++;
      sumtable[G(img,x,y)]=ImageGetPixel(img, x, y); //o valor de cada elemento da tabela das somas é igual ao valor de cinzento do pixel (x,y) correspondente
      if (x>0 && y>0){                               //+ o valor do elemento (x-1,y) + o valor do elemento (x,y-1) - o valor do elemento (x-1,y-1).
        sumtable[G(img,x,y)]-=sumtable[G(img,x-1,y-1)]; 
      }
      if (x > 0){
        sumtable[G(img,x,y)]+=sumtable[G(img,x-1,y)];
      }
      if (y > 0){
        sumtable[G(img,x,y)]+=sumtable[G(img,x,y-1)];
      } //As condições anteriores são necessárias para evitar que sejam somados pixeis que não pertencem à imagem
    }
  }//procede-se à aplicação do filtro de blur
  for (int x=0; x< img->width; x++){
    for (int y=0; y<img->height; y++){
      ITER++;
      int initial_x=MAX(x-dx,0); //evitar que as coordenadas da caixa do blur ultrapassem os limites da imagem
      int initial_y=MAX(y-dy,0);
      int x_end=MIN(x+dx, img->width-1); //evitar que as coordenadas da caixa do blur ultrapassem os limites da imagem
      int y_end=MIN(y+dy, img->height-1);
      int x_length=x_end-initial_x+1; //calcular o comprimento e a largura da caixa do blur
      int y_length=y_end-initial_y+1;
      int total=x_length*y_length; //numero de elementos da tabela de somas
      //Devido à dimensão da caixa fazem-se as seguintes operações para calcular o blur
      int blur=sumtable[G(img, x_end, y_end)]; //soma-se o valor do elemento (x,y) da tabela das somas
      if (initial_x > 0){
        blur-=sumtable[G(img,initial_x-1,y_end)]; //subtrai-se este elemento, pois este tem a soma do valor de pixeis fora dos limites do blur
      }
      if (initial_y > 0){
        blur-=sumtable[G(img,x_end,initial_y-1)]; //subtrai-se este elemento, pois este tem a soma do valor de pixeis fora dos limites do blur
      }
      if (initial_x > 0 && initial_y > 0){ 
        blur+=sumtable[G(img,initial_x-1,initial_y-1)]; //soma-se este elemento porque o valor dele é subtraido duas vezes nas condições anteriores
      }
      ImageSetPixel(img, x, y, (blur + total/2)/total); //atribui o valor de cinzento ao pixel, calculado através da expressão do blur
    }
  }
  free(sumtable);
  sumtable=NULL;
}

