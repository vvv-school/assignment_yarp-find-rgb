// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
//
// Assignment on how to use basic YARP classes for vision-based task.
//
// Author: Giulia Vezzani - <giulia.vezzani@iit.it>

#include <cstdlib>
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
    std::string m_ball_name{""};

    Vector ball_pos;
    Vector ball_col;

    /****************************************************/
    void createBall()
    {
        Bottle cmd,reply;
        cmd.addString("makeSphere");
        cmd.addFloat64(0.06); // radius
        cmd.addFloat64(ball_pos[0]); // pose x
        cmd.addFloat64(ball_pos[1]); // pose y
        cmd.addFloat64(ball_pos[2]); // pose z
        cmd.addFloat64(ball_pos[3]); // pose theta_0
        cmd.addFloat64(ball_pos[4]); // pose theta_1
        cmd.addFloat64(ball_pos[5]); // pose theta_2
        cmd.addInt16(static_cast<int16_t>(ball_col[0])); // R
        cmd.addInt16(static_cast<int16_t>(ball_col[1])); // G
        cmd.addInt16(static_cast<int16_t>(ball_col[2])); // B
        auto ok = port.write(cmd,reply);

        if(ok && reply.size()>0) {
            m_ball_name = reply.get(0).asString();
        }
        else {
            yError()<<"Ball not created!";
            return;
        }

        yDebug()<<"Ball color "<<ball_col.toString();
    }

    /****************************************************/
    void deleteBall()
    {
        Bottle cmd,reply;
        cmd.addString("deleteObject");
        cmd.addString(m_ball_name);
        port.write(cmd,reply);
    }

public:

    /****************************************************/
    bool configure(ResourceFinder &rf)
    {
        port.open("/ball");

        if (!Network::connect(port.getName(),"/world_input_port/yarp-find-rgb"))
        {
            yError()<<"Unable to connect to the world!";
            port.close();
            return false;
        }

        ball_pos.resize(6, 0.0);
        ball_pos[0] = -0.4;
        ball_pos[1] = 0.325;
        ball_pos[2] = 0.975;

        Rand::init();
        ball_col=255 * Rand::vector(3);

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
        return EXIT_FAILURE;
    }

    ResourceFinder rf;
    rf.configure(argc,argv);

    BallCreation ball;
    return ball.runModule(rf);
}
