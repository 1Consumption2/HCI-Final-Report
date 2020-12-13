/*===============================================================================
Copyright (c) 2020, PTC Inc. All rights reserved.
 
Vuforia is a trademark of PTC Inc., registered in the United States and other
countries.
===============================================================================*/

import UIKit
import MetalKit


class VuforiaView:UIView {

    var mVuforiaStarted = false
    private var mConfigurationChanged = true
    
    private var mRenderer: MetalRenderer!
    
    private var mMetalDevice:MTLDevice!
    private var mMetalCommandQueue:MTLCommandQueue!
    private var mCommandExecutingSemaphore:DispatchSemaphore!

    private var mDepthStencilState:MTLDepthStencilState!
    private var mDepthTexture:MTLTexture!

    private var mVideoBackgroundProjectionBuffer:MTLBuffer!
    private var mGuideViewModelViewProjectionBuffer:MTLBuffer!

    
    override class var layerClass: AnyClass {
        return CAMetalLayer.self
    }
    
    
    override init(frame: CGRect) {
        
        super.init(frame: frame)
        
        contentScaleFactor = UIScreen.main.nativeScale

        // Get the system default metal device
        mMetalDevice = MTLCreateSystemDefaultDevice()
        
        // Metal command queue
        mMetalCommandQueue = mMetalDevice.makeCommandQueue()
        
        // Create a dispatch semaphore, used to synchronise command execution
        self.mCommandExecutingSemaphore = DispatchSemaphore.init(value:1)

        // Create a CAMetalLayer and set its frame to match that of the view
        let layer = self.layer as! CAMetalLayer
        layer.device = mMetalDevice
        layer.pixelFormat = MTLPixelFormat.bgra8Unorm
        layer.framebufferOnly = true
        layer.contentsScale = self.contentScaleFactor
        
        // Get the default library from the bundle (Metal shaders)
        let library = mMetalDevice.makeDefaultLibrary()
        
        // Create a depth texture that is needed when rendering the augmentation.
        let screenSize = UIScreen.main.bounds.size
        let depthTextureDescriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: MTLPixelFormat.depth32Float, width: Int(screenSize.width * self.contentScaleFactor), height: Int(screenSize.height * self.contentScaleFactor), mipmapped: false)
        
        depthTextureDescriptor.usage = MTLTextureUsage.renderTarget
        mDepthTexture = mMetalDevice.makeTexture(descriptor: depthTextureDescriptor)
        
        // Video background projection matrix buffer
        mVideoBackgroundProjectionBuffer = mMetalDevice.makeBuffer(length: MemoryLayout<Float>.size * 16, options: [])
        // Guide view model view projection matrix buffer
        mGuideViewModelViewProjectionBuffer = mMetalDevice.makeBuffer(length: MemoryLayout<Float>.size * 16, options: [])

        // Fragment depth stencil
        let depthStencilDescriptor = MTLDepthStencilDescriptor()
        depthStencilDescriptor.depthCompareFunction = MTLCompareFunction.less
        depthStencilDescriptor.isDepthWriteEnabled = true
        self.mDepthStencilState = mMetalDevice.makeDepthStencilState(descriptor: depthStencilDescriptor)

        mRenderer = MetalRenderer(metalDevice: mMetalDevice, layer: layer, library: library, textureDepth: mDepthTexture)
    }
    
    
    required convenience init?(coder: NSCoder) {
        // This view fills the whole screen
        self.init(frame: UIScreen.main.bounds)
    }
    
    
    func configureVuforia() {
        let orientation = UIApplication.shared.statusBarOrientation
        
        var orientationValue:Int32
        switch orientation {
        case .portrait:
            orientationValue = 0 // "Portrait"
        case .portraitUpsideDown:
            orientationValue = 1 // "PortraitUpsideDown"
        case .landscapeLeft:
            orientationValue = 2 // "LandscapeLeft"
        case .landscapeRight:
            orientationValue = 3 // "LandscapeRight"
        case .unknown:
            orientationValue = 4 // "Default"
        @unknown default:
            orientationValue = 4 // "Default"
        }

        let screenSize = UIScreen.main.bounds.size
        configureRendering(
            Int32(screenSize.width * self.contentScaleFactor),
            Int32(screenSize.height * self.contentScaleFactor),
            orientationValue);
    }
    
    
    @objc func renderFrameVuforia() {
        objc_sync_enter(self)
        if (mVuforiaStarted) {
            
            if (mConfigurationChanged) {
                mConfigurationChanged = false
                configureVuforia()
            }
            
            renderFrameVuforiaInternal()
        }
        objc_sync_exit(self)
    }
    
    
    func renderFrameVuforiaInternal() {
        //Check if Camera is Started
        if (!isCameraStarted()) {
            return;
        }
        
        // ========== Set up ==========
        let layer = self.layer as! CAMetalLayer
        
        var viewport = MTLViewport(originX: 0.0, originY: 0.0, width: Double(layer.drawableSize.width), height: Double(layer.drawableSize.height), znear: 0.0, zfar: 1.0)
        var viewportsValue: Array<Double> = Array(arrayLiteral:
            0.0, 0.0,Double(layer.drawableSize.width), Double(layer.drawableSize.height),0.0, 1.0)
        // --- Command buffer ---
        // Get the command buffer from the command queue
        let commandBuffer = mMetalCommandQueue.makeCommandBuffer()
        
        // Get the next drawable from the CAMetalLayer
        let drawable = layer.nextDrawable()
        
        // It's possible for nextDrawable to return nil, which means a call to
        // renderCommandEncoderWithDescriptor will fail
        if (drawable == nil) {
            return;
        }
        // Wait for exclusive access to the GPU
        let _ = mCommandExecutingSemaphore.wait(timeout: DispatchTime.distantFuture)
        
        // -- Render pass descriptor ---
        // Set up a render pass decriptor
        let renderPassDescriptor = MTLRenderPassDescriptor()
        
        // Draw to the drawable's texture
        renderPassDescriptor.colorAttachments[0].texture = drawable?.texture
        // Clear the colour attachment in case there is no video frame
        renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadAction.clear
        // Store the data in the texture when rendering is complete
        renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreAction.store
        // Use textureDepth for depth operations.
        renderPassDescriptor.depthAttachment.texture = mDepthTexture;
        
        // Get a command encoder to encode into the command buffer
        let encoder = commandBuffer?.makeRenderCommandEncoder(descriptor: renderPassDescriptor)
        
        if (prepareToRender(&viewportsValue, UnsafeMutableRawPointer(Unmanaged.passRetained(mMetalDevice!).toOpaque()),
                            UnsafeMutableRawPointer(Unmanaged.passRetained(drawable!.texture).toOpaque()), UnsafeMutableRawPointer(Unmanaged.passRetained(encoder!).toOpaque()))) {
            viewport.originX = viewportsValue[0]
            viewport.originY = viewportsValue[1]
            viewport.width = viewportsValue[2]
            viewport.height = viewportsValue[3]
            viewport.znear = viewportsValue[4]
            viewport.zfar = viewportsValue[5]
            encoder?.setViewport(viewport)

            // Once the camera is initialized we can get the video background rendering values
            getVideoBackgroundProjection(mVideoBackgroundProjectionBuffer.contents())
            // Call the renderer to draw the video background
            mRenderer.renderVideoBackground(encoder: encoder, projectionMatrix: mVideoBackgroundProjectionBuffer, mesh: getVideoBackgroundMesh())

            encoder?.setDepthStencilState(mDepthStencilState)
            
            var worldOriginProjectionMatrix = matrix_float4x4()
            var worldOriginModelViewMatrix = matrix_float4x4()
            if (getOrigin(&worldOriginProjectionMatrix.columns, &worldOriginModelViewMatrix.columns)) {
                mRenderer.renderWorldOrigin(encoder: encoder, projectionMatrix: worldOriginProjectionMatrix, modelViewMatrix: worldOriginModelViewMatrix)
            }

            var trackableProjection = matrix_float4x4()
            var trackableModelView = matrix_float4x4()
            var trackableScaledModelView = matrix_float4x4()
            
            // Render image target bounding box if detected
            if (getImageTargetResult(&trackableProjection.columns, &trackableModelView.columns, &trackableScaledModelView.columns)) {
                mRenderer.renderImageTarget(encoder: encoder,
                                            projectionMatrix: trackableProjection,
                                            modelViewMatrix: trackableModelView,
                                            scaledModelViewMatrix: trackableScaledModelView)
            }

            var guideViewImage: VuforiaImage = VuforiaImage()
            // Render model target bounding box if detected, if not render guide view
            if (getModelTargetResult(&trackableProjection.columns, &trackableModelView.columns, &trackableScaledModelView.columns)) {
                mRenderer.renderModelTarget(encoder: encoder,
                                            projectionMatrix: trackableProjection,
                                            modelViewMatrix: trackableModelView,
                                            scaledModelViewMatrix: trackableScaledModelView)
                
            } else if (getModelTargetGuideView(mGuideViewModelViewProjectionBuffer.contents(), &guideViewImage)) {
                mRenderer.renderModelTargetGuideView(encoder: encoder, modelViewProjectionMatrix: mGuideViewModelViewProjectionBuffer, guideViewImage: &guideViewImage)
            }
        }
        
        // Pass Metal context data to Vuforia Engine (we may have changed the encoder since
        // calling Vuforia::Renderer::begin)
        finishRender(UnsafeMutableRawPointer(Unmanaged.passRetained(drawable!.texture).toOpaque()), UnsafeMutableRawPointer(Unmanaged.passRetained(encoder!).toOpaque()))
        
        // ========== Finish Metal rendering ==========
        encoder?.endEncoding()
        
        // Commit the rendering commands
        // Command completed handler
        commandBuffer?.addCompletedHandler { _ in self.mCommandExecutingSemaphore.signal()}
        
        // Present the drawable when the command buffer has been executed (Metal
        // calls to CoreAnimation to tell it to put the texture on the display when
        // the rendering is complete)
        commandBuffer?.present(drawable!)
        
        // Commit the command buffer for execution as soon as possible
        commandBuffer?.commit()
    }

}
