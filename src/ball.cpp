// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
//
// Assignment on how to use basic YARP classes for vision-based task.
//
// Author: Giulia Vezzani - <giulia.vezzani@iit.it>


#include <cmath>
#include <string>

#include <yarp/os/Network.h>
#include <yarp/os/Time.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/Bottle.h>
#include <yarp/os/RpcClient.h>
#include <yarp/sig/Vector.h>
#include <yarp/math/Math.h>
#include <yarp/math/Rand.h>

#include <iCub/ctrl/pids.h>

using namespace std;
using namespace yarp::os;
using namespace yarp::sig;
using namespace yarp::math;
using namespace iCub::ctrl;

class BallCreation : public RFModule
{
private:
    RpcClient port;

    Vector ball_pos;
    Vector ball_col;

    /****************************************************/
    void createBall()
    {
        Bottle cmd,reply;
        cmd.addString("world");
        cmd.addString("mk");
        cmd.addString("ssph");
        cmd.addDouble(0.06);
        cmd.addDouble(ball_pos[0]);
        cmd.addDouble(ball_pos[1]);
        cmd.addDouble(ball_pos[2]);
        cmd.addDouble(ball_col[0]);
        cmd.addDouble(ball_col[1]);
        cmd.addDouble(ball_col[2]);
        port.write(cmd,reply);

        yDebug()<<"Ball color "<<(ball_col*255).toString();
    }

    /****************************************************/
    void deleteBall()
    {
        Bottle cmd,reply;
        cmd.addString("world");
        cmd.addString("del");
        cmd.addString("all");

        port.write(cmd,reply);
    }

public:

    /****************************************************/
    bool configure(ResourceFinder &rf)
    {
        port.open("/ball");

        if (!Network::connect(port.getName().c_str(),"/icubSim/world"))
        {
            yError()<<"Unable to connect to the world!";
            port.close();
            return false;
        }

        ball_pos.resize(3);
        ball_pos[0]=-0.4;
        ball_pos[1]=0.925;
        ball_pos[2]=0.35;

        Rand::init();
        ball_col=Rand::vector(3);

        createBall();

        return true;
    }

    /****************************************************/
    bool close()
    {
        deleteBall();
        port.close();
        return true;
    }

    /****************************************************/
    double getPeriod()
    {
        return 0.02;
    }

    /****************************************************/
    bool updateModule()
    {
        return true;
    }
};

int main(int argc, char *argv[])
{
    Network yarp;
    if (!yarp.checkNetwork())
    {
        yError()<<"YARP doesn't seem to be available";
        return 1;
    }

    ResourceFinder rf;
    rf.configure(argc,argv);

    BallCreation ball;
    return ball.runModule(rf);
}
