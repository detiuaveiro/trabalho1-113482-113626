// imageTest - A program that performs some image processing.
//
// This program is an example use of the image8bit module,
// a programming project for the course AED, DETI / UA.PT
//
// You may freely use and modify this code, NO WARRANTY, blah blah,
// as long as you give proper credit to the original and subsequent authors.
//
// João Manuel Rodrigues <jmr@ua.pt>
// 2023

#include <assert.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image8bit.h"
#include "instrumentation.h"

#define PIXMEM InstrCount[0]
#define PIXCOMP InstrCount[1]

void MassSetting(Image img, uint8 value){
  for (int x = 0; x < ImageWidth(img); x++){
    for (int y = 0; y < ImageHeight(img); y++){
      ImageSetPixel(img, x, y, value);
    }
  }
}

int main(int argc, char* argv[]) {
  int px;
  int py;
  int res;

  for (int i = 1; i < argc; i++){

    ImageInit();

    printf("\n================================== Imagem %d ====================================\n",i);
    
    printf("\n# LOAD NEW image : %s\n", argv[i]);
    InstrReset(); // to reset instrumentation
    Image img1 = ImageLoad(argv[i]);
    int n = PIXMEM;
    if (img1 == NULL) {
      error(2, errno, "Loading %s: %s", argv[i], ImageErrMsg());
    }
    InstrPrint();
    
    fflush(stdout); // to print instrumentation
    Image cp1 = ImageCrop(img1, 0, 0, ImageWidth(img1), ImageHeight(img1));

    /*printf("\n===========\n");
    Image nb1 = ImageCrop(cp1, 0, 0, ImageWidth(img1), ImageHeight(img1));//aqui
    printf("\n# Não otimizado:\n # BLUR image (size: %d - window %dx%d) com dx = %d e dy = %d\n", n,ImageWidth(img1),ImageHeight(img1),10,10);
    InstrReset();
    ImageBlur2(nb1,10,10);
    InstrPrint();

    ImageDestroy(&nb1);

    nb1 = ImageCrop(cp1, 0, 0, ImageWidth(img1), ImageHeight(img1));//aqui
    printf("\n# Não otimizado:\n # BLUR image (size: %d - window %dx%d) com dx = %d e dy = %d\n", n,ImageWidth(img1),ImageHeight(img1),20,20);
    InstrReset();
    ImageBlur2(nb1,20,20);
    InstrPrint();

    ImageDestroy(&nb1);*/

    Image nb1 = ImageCrop(cp1, 0, 0, ImageWidth(img1), ImageHeight(img1));//74
    printf("\n# Otimizado:\n # BLUR image (size: %d - window %dx%d) com dx = %d e dy = %d\n", n,ImageWidth(img1),ImageHeight(img1),10,10);
    InstrReset();
    ImageBlur(nb1,10,10);
    InstrPrint();

    ImageDestroy(&nb1);

    nb1 = ImageCrop(cp1, 0, 0, ImageWidth(img1), ImageHeight(img1));//84
    printf("\n# Otimizado:\n # BLUR image (size: %d - window %dx%d) com dx = %d e dy = %d\n", n,ImageWidth(img1),ImageHeight(img1),20,20);
    InstrReset();
    ImageBlur(nb1,20,20);
    InstrPrint();

    ImageDestroy(&nb1);

    printf("\n===========\n");
    InstrReset();


    nb1 = ImageCrop(cp1, 0, 0, ImageWidth(img1), ImageHeight(img1));
    MassSetting(nb1, 100);

    InstrReset();
    /*for(int i = 5; i>=1; i--){
      Image crop = ImageCrop(nb1,0,0,(int)ImageWidth(nb1)/i,(int)ImageHeight(nb1));
      int size = (int)ImageWidth(crop) * (int)ImageHeight(crop);
      InstrReset();
      printf("\n# Não otimizado:\n # Locate image (size: %d - window %dx%d) in image (size: %d - window %dx%d)\n",size,(int)ImageWidth(crop),(int)ImageHeight(crop), n,ImageWidth(img1),ImageHeight(img1));
      res = ImageLocateSubImage2(nb1,&px,&py,crop);
      printf("\n# Best Case = encontra (Sucess = %d FOUND(%d,%d))\n",res,px,py);
      InstrPrint();
      ImageSetPixel(crop,ImageWidth(nb1)/i-1,ImageHeight(nb1)/i-1,2);
      InstrReset();
      res = ImageLocateSubImage2(nb1,&px,&py,crop);
      printf("\n# Worst Case = não encontra (Sucess = %d NOT FOUND)\n",res);
      InstrPrint();
      ImageDestroy(&crop);
    }*/
    ImageDestroy(&nb1);

    nb1 = ImageCrop(cp1, 0, 0, ImageWidth(img1), ImageHeight(img1));
    MassSetting(nb1, 100);

    InstrReset();
    for(int i = 5; i>=1; i--){
      Image crop = ImageCrop(nb1,0,0,(int)ImageWidth(nb1)/i,(int)ImageHeight(nb1));
      int size = (int)ImageWidth(crop) * (int)ImageHeight(crop);
      InstrReset();
      printf("\n# Otimizado:\n # Locate image (size: %d - window %dx%d) in image (size: %d - window %dx%d)\n",size,(int)ImageWidth(crop),(int)ImageHeight(crop), n,ImageWidth(img1),ImageHeight(img1));
      res = ImageLocateSubImage(nb1,&px,&py,crop);
      printf("\n# Best Case ?=? encontra (Sucess = %d FOUND(%d,%d))\n",res,px,py);
      InstrPrint();
      InstrReset();
      MassSetting(crop,101);
      res = ImageLocateSubImage(nb1,&px,&py,crop);
      printf("\n# Best Case ?=? não encontra (Sucess = %d NOTFOUND)\n",res);
      InstrPrint();
      InstrReset();
      ImageDestroy(&crop);
    }
    InstrReset();

    ImageDestroy(&img1);
    ImageDestroy(&cp1);
    ImageDestroy(&nb1);
  }
 return 0;
}
