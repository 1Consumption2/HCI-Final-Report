/*===============================================================================
Copyright (c) 2019 PTC Inc. All Rights Reserved.

Copyright (c) 2014 Qualcomm Connected Experiences, Inc. All Rights Reserved.

Vuforia is a trademark of PTC Inc., registered in the United States and other 
countries.

\file
    Recorder.h

\brief
    Header file for Recorder class.
===============================================================================*/

#ifndef _VUFORIA_RECORDER_H_
#define _VUFORIA_RECORDER_H_

// Include files
#include <Vuforia/NonCopyable.h>
#include <Vuforia/VideoMode.h>

namespace Vuforia
{

/// Records video and sensor data.
class VUFORIA_API Recorder : private NonCopyable
{
public:

    enum SensorsRecording
    {
        Camera = 0x01,
        Accelerometer = 0x02,
        Gyroscope = 0x04, 
        Magnetometer = 0x08,
        DevicePose = 0x10
    };
    
    enum ImageScale
    {
        ScaleUnknown,
        ScaleOne,
        ScaleHalf
    };

    enum OutputFileMode
    {
        OutputFileModeUnknown,
        OutputFileModeUncompressed,
        OutputFileModeZip
    };

    /// Returns the Recorder singleton instance.
    static Recorder& getInstance();

    /// Initialize the recorder.
    /**
     *
     * \returns true if the recorder was successfully initialized with all the
     * requested sensors, or false otherwise (check application logs for details).
     */
    virtual bool init(const char* path = 0, int flags = (Vuforia::Recorder::Camera | Vuforia::Recorder::Accelerometer
                                                        | Vuforia::Recorder::Gyroscope | Vuforia::Recorder::Magnetometer
                                                        | Vuforia::Recorder::DevicePose)) = 0;

    /// Deinitialize the recorder.
    /**
     * Release any resources created or used by the recorder.
     *
     * \note This function should not be called during the execution of the
     * UpdateCallback.
     *
     * \returns true true on succes, or false otherwise
     * (check application logs for details).
     */
    virtual bool deinit() = 0;

    /// Start the recorder.
    /**
     * This method starts the process of recording data from the requested sensors.
     * The Recorder must have been initialized first via a call to init().
     *
     * \returns true if the recorder was successfully able to start the
     * recording of all requested sensors, or false otherwise
     * (check application logs for details).
     */
    virtual bool start() = 0;

    /// Stop the recorder.
    /**
     * Stop recording data from the requested sensors.
     *
     * \returns true if the recorder was successfully able to stop the
     * recording of all requested sensors, or false otherwise
     * (check application logs for details).
     */
    virtual bool stop()  = 0;
    
    /// Get the path to the current recording.
    /**
     * Get the internally assembled directory name for the current
     * recording. The Recorder must have been started first via a
     * call to start().
     *
     * \note The returned pointer will be invalidated at the next
     * time start() is called, or if deinit() is called.
     *
     * \returns a valid string on success, null otherwise.
     */    
    virtual const char * getRecordingPath() = 0;

    /// Get supported camera video mode(s).
    /**
     * \note This method always returns null if camera recording has not
     * been requested successfully in the init() method.
     *
     * \param videoModes If not null, contains the supported camera
     * video mode(s) after this function returns.
     *
     * \returns the number of available VideoModes.
     */
    virtual int getSupportedResolutions(Vuforia::VideoMode* videoModes) = 0;

    /// Get the supported sensors.
    /**
     * Retrieve the device specific list of supported sensors (accelerometer,
     * gyroscope, magnetometer).
     *
     * \note Use this method as a helper to assemble the flags for the
     * init() method.
     *
     * \returns a bitfield as defined by SensorsRecording
     */
    virtual int getSupportedSensors() const = 0;
    
    /// Set custom framerate for camera image recording.
    /**
     * Tell the recorder to capture camera frames at a framerate lower
     * than the one the camera is using.
     *
     * \note This method can only be called when the recorder
     * is initialized but not started.
     *
     * \returns true on succes, or false otherwise
     * (check application logs for details).
     */
    virtual bool setFramerate(float framerate) = 0;
    
    /// Get the custom framerate for camera image recording.
    /**
     *
     * \returns the current custom framerate of the recorder
     * or -1.
     */
    virtual float getFramerate() const = 0;
    
    /// Set image rescale factor for camera image recording.
    /**
     * Tell the recorder to capture camera frames at a reduced resolution.
     *
     * \note This method can only be called when the recorder
     * is initialized but not started.
     *
     * \returns true on succes, or false otherwise
     * (check application logs for details).
     */
    virtual bool setImageScale(ImageScale scale) = 0;
    
    /// Get the image rescale factor for camera image recording.
    /**
     *
     * \returns the current image rescale factor of the recorder
     * or ScaleUnknown.
     */
    virtual ImageScale getImageScale() const = 0;

    /// Set output file mode for camera image recording.
    /**
    * Tell the recorder to save the camera frames in a specific mode
    *
    * \note This method can only be called after the recorder has been initialized
    * is initialized but not started
    *
    * \returns true on success, or false otherwise
    */
    virtual bool setRecordingOutputFileMode(OutputFileMode mode) = 0;

    /// Get the output file mode for camera image recording.
    /**
     *
     * \returns the current output file mode for camera image recording
     */
    virtual OutputFileMode getRecordingOutputFileMode() const = 0;
};

} // namespace Vuforia

#endif // _VUFORIA_RECORDER_H_
