/*===============================================================================
Copyright (c) 2020, PTC Inc. All rights reserved.

Vuforia is a trademark of PTC Inc., registered in the United States and other
countries.
===============================================================================*/

#include "VuforiaWrapper.h"

#include "AppController.h"
#include "MathUtils.h"
#include "MemoryStream.h"
#include "Models.h"
#include "tiny_obj_loader.h"

#include <Vuforia/iOS/MetalRenderer.h>

#include <vector>

AppController controller;

struct
{
    void* callbackClass = nullptr;
    void(*errorCallbackMethod)(void *, const char *) = nullptr;
    void(*initDoneCallbackMethod)(void *) = nullptr;

} gWrapperData;


/// Method to load obj model files, uses C++ so outside extern block
bool loadObjModel(const char * const data, int dataSize,
                  int& numVertices, float** vertices, float** texCoords);


extern "C"
{

int VUFORIA_INIT_FLAG_METAL = Vuforia::INIT_FLAGS::METAL;


int getImageTargetId()
{
    return AppController::IMAGE_TARGET_ID;
}


int getModelTargetId()
{
    return AppController::MODEL_TARGET_ID;
}


void initAR(VuforiaInitConfig config, int target)
{
    // Hold onto pointers for later use by the lambda passed to initAR below
    gWrapperData.callbackClass = config.classPtr;
    gWrapperData.errorCallbackMethod = config.errorCallback;
    gWrapperData.initDoneCallbackMethod = config.initDoneCallback;

    // Create InitConfig structure and populate...
    AppController::InitConfig initConfig;
    initConfig.vuforiaInitFlags = config.initFlags;
    initConfig.showErrorCallback = [](const char *errorString) {
        gWrapperData.errorCallbackMethod(gWrapperData.callbackClass, errorString);
    };
    initConfig.initDoneCallback = [](){
        gWrapperData.initDoneCallbackMethod(gWrapperData.callbackClass);
    };
    
    // Call AppController to initialize Vuforia ...
    controller.initAR(initConfig, target);
}


bool startAR()
{
    return controller.startAR();
}


void pauseAR()
{
    controller.pauseAR();
}


void resumeAR()
{
    controller.resumeAR();
}


void stopAR()
{
    controller.stopAR();
}


void deinitAR()
{
    controller.deinitAR();
}


bool isCameraStarted()
{
    return controller.isCameraStarted();
}


void cameraPerformAutoFocus()
{
    controller.cameraPerformAutoFocus();
}


void cameraRestoreAutoFocus()
{
    controller.cameraRestoreAutoFocus();
}


void configureRendering(int width, int height, int orientation)
{
    controller.configureRendering(width, height, orientation);
}


bool prepareToRender(double* viewport, void* metalDevice, void* texture, void* encoder)
{
    static Vuforia::MetalRenderData renderData;
    renderData.mData.drawableTexture = (__bridge id<MTLTexture>)texture;
    renderData.mData.commandEncoder = (__bridge id<MTLRenderCommandEncoder>)encoder;

    static Vuforia::MetalTextureUnit unit;
    unit.mTextureIndex = 0;

    return controller.prepareToRender(viewport, &renderData, &unit);
}


void finishRender(void* texture, void* encoder)
{
    static Vuforia::MetalRenderData renderData;
    renderData.mData.drawableTexture = (__bridge id<MTLTexture>)texture;
    renderData.mData.commandEncoder = (__bridge id<MTLRenderCommandEncoder>)encoder;

    controller.finishRender(&renderData);
}


// contents is a 16 element float array
void getVideoBackgroundProjection(void *mvp)
{
    auto rp = controller.getRenderingPrimitives();
    assert(rp);
    auto vbProjection = rp->getVideoBackgroundProjectionMatrix(Vuforia::VIEW_SINGULAR);

    memset(mvp, 0, 16 * sizeof(float));
    memcpy(mvp, vbProjection.data, sizeof(vbProjection.data));

    // The caller expects a 4x4 matrix with the last element set to 1.0
    // RenderingPrimitives provided a 3x4 matrix so we need to fill this in
    float* contentsFloat = static_cast<float*>(mvp);
    contentsFloat[15] = 1.0f;
}


VuforiaMesh getVideoBackgroundMesh()
{
    auto rp = controller.getRenderingPrimitives();
    assert(rp);
    const Vuforia::Mesh& vbMesh = rp->getVideoBackgroundMesh(Vuforia::VIEW_SINGULAR);
    
    VuforiaMesh result = {
        vbMesh.getNumVertices(),
        vbMesh.getPositionCoordinates(),
        vbMesh.getUVCoordinates(),
        vbMesh.getNumTriangles() * 3,
        vbMesh.getTriangles(),
    };
    
    return result;
}

bool getOrigin(void* projection, void* modelView)
{
    Vuforia::Matrix44F projectionMat44;
    Vuforia::Matrix44F modelViewMat44;
    if (controller.getOrigin(projectionMat44, modelViewMat44))
    {
        memcpy(projection, &projectionMat44.data, sizeof(projectionMat44.data));
        memcpy(modelView, &modelViewMat44.data, sizeof(modelViewMat44.data));
        return true;
    }
    
    return false;
}


bool getImageTargetResult(void* projection, void* modelView, void* scaledModelView)
{
    Vuforia::Matrix44F projectionMatrix;
    Vuforia::Matrix44F modelViewMatrix;
    Vuforia::Matrix44F scaledModelViewMatrix;
    if (controller.getImageTargetResult(projectionMatrix, modelViewMatrix, scaledModelViewMatrix))
    {
        memcpy(projection, &projectionMatrix.data, sizeof(projectionMatrix.data));
        memcpy(modelView, &modelViewMatrix.data, sizeof(modelViewMatrix.data));
        memcpy(scaledModelView, &scaledModelViewMatrix.data, sizeof(scaledModelViewMatrix.data));

        return true;
    }

    return false;
}


bool getModelTargetResult(void* projection, void* modelView, void* scaledModelView)
{
    Vuforia::Matrix44F projectionMatrix;
    Vuforia::Matrix44F modelViewMatrix;
    Vuforia::Matrix44F scaledModelViewMatrix;
    if (controller.getModelTargetResult(projectionMatrix, modelViewMatrix, scaledModelViewMatrix))
    {
        memcpy(projection, &projectionMatrix.data, sizeof(projectionMatrix.data));
        memcpy(modelView, &modelViewMatrix.data, sizeof(modelViewMatrix.data));
        memcpy(scaledModelView, &scaledModelViewMatrix.data, sizeof(scaledModelViewMatrix.data));

        return true;
    }

    return false;
}


bool getModelTargetGuideView(void* mvp, VuforiaImage* guideViewImage)
{
    Vuforia::Matrix44F projection;
    Vuforia::Matrix44F modelView;
    Vuforia::Image* image = nullptr;
    if (controller.getModelTargetGuideView(projection, modelView, &image))
    {
        Vuforia::Matrix44F modelViewProjection;
        MathUtils::multiplyMatrix(projection, modelView, modelViewProjection);
        memcpy(mvp, &modelViewProjection.data, sizeof(modelViewProjection.data));

        guideViewImage->width = image->getBufferWidth();
        guideViewImage->height = image->getBufferHeight();
        guideViewImage->stride = image->getStride();
        guideViewImage->underlying = image;

        return true;
    }

    return false;
}


bool getImagePixels(VuforiaImage* guideViewImage, void* buffer, int bufferSize)
{
    Vuforia::Image* underlyingImage = static_cast<Vuforia::Image*>(guideViewImage->underlying);

    int size = underlyingImage->getBufferHeight() * underlyingImage->getStride();
    if (bufferSize < size)
    {
        return false;
    }
    
    memcpy(buffer, underlyingImage->getPixels(), size);

    return true;
}


VuforiaModel loadModel(const char * const data, int dataSize)
{
    int numVertices = 0;
    float* rawVertices = nullptr;
    float* rawTexCoords = nullptr;
  
    bool ret = loadObjModel(data, dataSize, numVertices, &rawVertices, &rawTexCoords);

    return VuforiaModel {
        ret,
        numVertices,
        rawVertices,
        rawTexCoords,
    };
}


void releaseModel(VuforiaModel* model)
{
    model->isLoaded = false;
    model->numVertices = 0;
    delete[] model->vertices;
    model->vertices = nullptr;
    delete[] model->textureCoordinates;
    model->textureCoordinates = nullptr;
}


// Map the static Model data into the struct instance exposed to Swift
Models_t Models =
{
    NUM_SQUARE_VERTEX,
    NUM_SQUARE_INDEX,
    NUM_SQUARE_WIREFRAME_INDEX,
    squareVertices,
    squareTexCoords,
    squareIndices,
    squareWireframeIndices,
    NUM_CUBE_VERTEX,
    NUM_CUBE_INDEX,
    NUM_CUBE_WIREFRAME_INDEX,
    cubeVertices,
    cubeTexCoords,
    cubeIndices,
    cubeWireframeIndices,
    NUM_AXIS_INDEX,
    NUM_AXIS_VERTEX,
    NUM_AXIS_COLOR,
    axisVertices,
    axisColors,
    axisIndices,
};

} // extern "C"

bool loadObjModel(const char * const data, int dataSize,
                  int& numVertices, float** vertices, float** texCoords)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    MemoryInputStream aFileDataStream(data, dataSize);
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, &aFileDataStream);
    if (ret && err.empty())
    {
        numVertices = 0;
        std::vector<float> vecVertices;
        std::vector<float> vecTexCoords;

        // Loop over shapes
        // s is the index into the shapes vector
        // f is the index of the current face
        // v is the index of the current vertex
        for (size_t s = 0; s < shapes.size(); ++s)
        {
            // Loop over faces(polygon)
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f)
            {
                int fv = shapes[s].mesh.num_face_vertices[f];
                numVertices += fv;

                // Loop over vertices in the face.
                for (size_t v = 0; v < fv; ++v)
                {
                    // access to vertex
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                    vecVertices.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
                    vecVertices.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
                    vecVertices.push_back(attrib.vertices[3 * idx.vertex_index + 2]);

                    // The model may not have texture coordinates for every vertex
                    // If a texture coordinate is missing we just set it to 0,0
                    // This may not be suitable for rendering some OBJ model files
                    if (idx.texcoord_index < 0)
                    {
                        vecTexCoords.push_back(0.f);
                        vecTexCoords.push_back(0.f);
                    }
                    else
                    {
                        vecTexCoords.push_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
                        vecTexCoords.push_back(attrib.texcoords[2 * idx.texcoord_index + 1]);
                    }
                }
                index_offset += fv;
            }
        }

        *vertices = new float[vecVertices.size() * 3];
        memcpy(*vertices, vecVertices.data(), vecVertices.size() * sizeof(float));
        *texCoords = new float[vecTexCoords.size() * 2];
        memcpy(*texCoords, vecTexCoords.data(), vecTexCoords.size() * sizeof(float));
    }
    
    return ret;
}
