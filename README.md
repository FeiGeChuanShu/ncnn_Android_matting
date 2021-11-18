# ncnn_Android_modnet
Android human matting demo infer by ncnn  

the model from **MODNet: Trimap-Free Portrait Matting in Real Time**  

## how to build and run
### step1
https://github.com/Tencent/ncnn/releases

* Download ncnn-YYYYMMDD-android-vulkan.zip or build ncnn for android yourself
* Extract ncnn-YYYYMMDD-android-vulkan.zip into **app/src/main/jni** and change the **ncnn_DIR** path to yours in **app/src/main/jni/CMakeLists.txt**

### step2
https://github.com/nihui/opencv-mobile

* Download opencv-mobile-XYZ-android.zip
* Extract opencv-mobile-XYZ-android.zip into **app/src/main/jni** and change the **OpenCV_DIR** path to yours in **app/src/main/jni/CMakeLists.txt**

### step3
* Open this project with Android Studio, build it and enjoy!  

## result  
![](test_img.jpg)  
![](screenshot.png)  

## reference  
1.https://github.com/nihui/ncnn-android-styletransfer  
2.https://github.com/ZHKKKe/MODNet  
3.https://github.com/PaddlePaddle/PaddleSeg/tree/release/2.3/contrib/Matting