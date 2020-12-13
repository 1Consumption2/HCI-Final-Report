/*===============================================================================
Copyright (c) 2020, PTC Inc. All rights reserved.
 
Vuforia is a trademark of PTC Inc., registered in the United States and other
countries.
===============================================================================*/
#import <Foundation/Foundation.h>

#ifdef __cplusplus
extern "C"
{
#endif

extern int VUFORIA_INIT_FLAG_METAL;

/// Vuforia initialization parameter structure for Swift
typedef struct
{
    void * classPtr;
    void(*errorCallback)(void *, const char *);
    void(*initDoneCallback)(void *);
    int initFlags;
} VuforiaInitConfig;


/// Vuforia::Mesh repesentation for Swift
typedef struct
{
    const int numVertices;
    const float* vertices;
    const float* textureCoordinates;
    const int numIndices;
    const unsigned short* indices;
} VuforiaMesh;


/// Vuforia::Image representation for Swift
typedef struct
{
    int width;
    int height;
    int stride;
    void* underlying; // The Vuforia::Image instance that this wraps
} VuforiaImage;


/// Modelv3d representation for Swift
typedef struct
{
    bool isLoaded;
    int numVertices;
    const float* vertices;
    const float* textureCoordinates;
} VuforiaModel;


int getImageTargetId();
int getModelTargetId();

void initAR(VuforiaInitConfig config, int target);
bool startAR();
void pauseAR();
void resumeAR();
void stopAR();
void deinitAR();

bool isCameraStarted();
void cameraPerformAutoFocus();
void cameraRestoreAutoFocus();

void configureRendering(int width, int height, int orientation);

bool prepareToRender(double* viewport, void* metalDevice, void* texture, void* encoder);
void finishRender(void* texture, void* encoder);

void getVideoBackgroundProjection(void* mvp);
VuforiaMesh getVideoBackgroundMesh();

bool getOrigin(void* projection, void* modelView);
bool getImageTargetResult(void* projection, void* modelView, void* scaledModelView);
bool getModelTargetResult(void* projection, void* modelView, void* scaledModelView);
bool getModelTargetGuideView(void* mvp, VuforiaImage* guideViewImage);
bool getImagePixels(VuforiaImage* guideViewImage, void* buffer, int bufferSize);
VuforiaModel loadModel(const char * const data, int dataSize);
void releaseModel(VuforiaModel* model);

typedef struct
{
    const unsigned short NUM_SQUARE_VERTEX;
    const unsigned short NUM_SQUARE_INDEX;
    const unsigned short NUM_SQUARE_WIREFRAME_INDEX;
    const float* squareVertices;
    const float* squareTexCoords;
    const unsigned short* squareIndices;
    const unsigned short* squareWireframeIndices;

    const unsigned short NUM_CUBE_VERTEX;
    const unsigned short NUM_CUBE_INDEX;
    const unsigned short NUM_CUBE_WIREFRAME_INDEX;
    const float* cubeVertices;
    const float* cubeTexCoords;
    const unsigned short* cubeIndices;
    const unsigned short* cubeWireframeIndices;

    const unsigned short NUM_AXIS_INDEX;
    const unsigned short NUM_AXIS_VERTEX;
    const unsigned short NUM_AXIS_COLOR;
    const float* axisVertices;
    const float* axisColors;
    const unsigned short* axisIndices;

} Models_t;
/// Instance of the struct populated with model data for use in Swift
extern Models_t Models;

#ifdef __cplusplus
};
#endif
