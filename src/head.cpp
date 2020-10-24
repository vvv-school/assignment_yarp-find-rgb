// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
//
// Assignment on how to use basic YARP classes for vision-based task.
//
// Author: Giulia Vezzani - <giulia.vezzani@iit.it>

#include <cstdlib>
#include <array>
#include <string>
#include <fstream>
#include <vector>

#include <yarp/os/Network.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/Bottle.h>
#include <yarp/sig/Image.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/RpcClient.h>
#include <yarp/os/Time.h>

#include <yarp/dev/Drivers.h>
#include <yarp/dev/IControlMode.h>
#include <yarp/dev/IEncoders.h>
#include <yarp/dev/IPositionControl.h>
#include <yarp/dev/PolyDriver.h>

#include <iCub/ctrl/math.h>

using namespace std;
using namespace yarp::os;
using namespace yarp::sig;
using namespace yarp::dev;

using namespace iCub::ctrl;

class HeadMover : public RFModule
{
protected:
    // driver and interfaces for controlling the head
    PolyDriver                      driver;
    IControlMode                   *imod;
    IEncoders                      *ienc;
    IPositionControl               *ipos;

    // angle used to move one head joint (guess which one! :))
    double                          angle;
    // number of head axes
    int                             nAxes;

    // ports required from the module
    BufferedPort<ImageOf<PixelRgb>> imagePort;      // read image from one robot camera
    BufferedPort<Bottle>            anglePort;      // receive the angle for moving the head
    BufferedPort<Bottle>            colorPort;      // send the RGB code of the center of the image

    bool go_on;

    /****************************************************/
    bool moveHead()
    {
        bool moved=true;
        // send commands to the robot head to move the correct joint
        // hint: correctly update the moved variable according to the control output
        // FILL IN THE CODE

        return moved;
    }

    /****************************************************/
    bool findColor()
    {
        // read from robot camera
        ImageOf<PixelRgb> *image=imagePort.read(false);

        // check if any image is received
        if (image!=NULL)
        {
            // get the center of the image
            // hint: class image stores information about its dimensions
            double center_u=0.0;    //FILL IN THE CODE
            double center_v=0.0;

            // get the rgb pixel of the image center
            // hint: you can use PixelRgb class
            // FILL IN THE CODE

            // send pixel RGB color to the dedicated port
            // hint: use YARP Bottle class and consider RGB values
            // as Double.            
            Bottle &color=colorPort.prepare();
            // FILL IN THE CODE

            colorPort.write();
        }

        return true;
    }

    /****************************************************/
    bool configPorts()
    {
        bool ret=true;

        // open all ports and check that everything is fine
        ret = colorPort.open("/head/color:o");
        ret = ret && imagePort.open("/head/img:i");
        ret = ret && anglePort.open("/head/ang:i");

        return ret;
    }

    /****************************************************/
    bool configDevice()
    {
        // configure the options for the driver
        Property option;
        option.put("device","remote_controlboard");
        option.put("remote","/icubSim/head");
        option.put("local","/controller");

        // open the driver
        if (!driver.open(option))
        {
            yError()<<"Unable to open the device driver";
            return false;
        }

        // open the views
        driver.view(imod);
        driver.view(ienc);
        driver.view(ipos);

        // tell the device we aim to control
        // in position mode all the joints
        ienc->getAxes(&nAxes);
        // FILL IN THE CODE

        return true;
    }

public:
    /****************************************************/
    bool configure(ResourceFinder &rf)
    {
        bool config_ok;

        Time::delay(3.0);

        // configure the device for controlling the head
        config_ok = configDevice();

        // configure the ports
        config_ok = config_ok && configPorts();

        // configure updateModule logic
        go_on=false;

        return config_ok;
    }

    /****************************************************/
    double getPeriod()
    {
        return 0.1;
    }

    /****************************************************/
    bool close()
    {
        // close all ports after checking if they are closed
        if (!colorPort.isClosed())
            colorPort.close();
        if (!anglePort.isClosed())
            anglePort.close();
        if (!imagePort.isClosed())
            imagePort.close();

        return true;
    }

    /****************************************************/
    bool interrupt()
    {
        // interrupt image port
        imagePort.interrupt();
        return true;
    }

    /****************************************************/
    bool updateModule()
    {
        // wait for the angle to be used for moving the head
        go_on=getAngle();

        // move the head
        if (go_on)
            go_on=moveHead();

        // return the detected color
        findColor();

        return true;
    }

   /****************************************************/
    bool getAngle()
    {
        // read a bottle containing the angle
        // note: do you want the port to wait or not for an answer?
        Bottle *angle_bottle;//= FILL IN THE CODE

        // get the information for angle_bottle
        if (angle_bottle!=NULL)
        {
            // check if the bottle is correctly filled:
            // the test will send to you code the following messagge:
            // string ("angle") + double
            // how do you read it?
            // FILL IN THE CODE
            return true;
        }

        return false;
    }
};


int main(int argc, char *argv[])
{
    Network yarp;
    if (!yarp.checkNetwork())
    {
        yError()<<"YARP doesn't seem to be available";
        return EXIT_FAILURE;
    }

    ResourceFinder rf;
    rf.setDefaultContext("assignment_yarp-find-rgb");
    rf.configure(argc,argv);

    HeadMover mod;
    return mod.runModule(rf);
}
