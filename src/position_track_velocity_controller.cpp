#include <ros/ros.h>
#include <vector>
#include <iterator>
#include <string>

#include <dji_sdk/dji_sdk.h>
#include <dji_sdk/GimbalAngleControl.h>
#include <dji_sdk/GimbalSpeedControl.h>
#include <dji_sdk/VelocityControl.h>

#include <std_msgs/Float64.h>
#include <std_msgs/Bool.h>

#include <geometry_msgs/PoseArray.h>

#include <cmath>
#include <iostream>

#include <Eigen/Geometry> 

ros::Subscriber velocity_control_x_sub;
ros::Subscriber velocity_control_y_sub;
ros::Subscriber velocity_control_yaw_sub;
ros::Subscriber position_track_enable_sub;
ros::Subscriber landing_condition_met_sub;

ros::ServiceClient velocity_control_service;
ros::ServiceClient sdk_permission_control_service;


double velocity_control_effort_x;
double velocity_control_effort_y;
double velocity_control_effort_yaw;

const double descending_speed = -0.5;

bool position_track_enabled = false;
bool landing_condition_met = false;

void velocityControlEffortXCallback(std_msgs::Float64 velocity_control_effort_x_msg)
{
	velocity_control_effort_x = velocity_control_effort_x_msg.data;
}

void velocityControlEffortYCallback(std_msgs::Float64 velocity_control_effort_y_msg)
{
	velocity_control_effort_y = velocity_control_effort_y_msg.data;
}

void velocityControlEffortYawCallback(std_msgs::Float64 velocity_control_effort_yaw_msg)
{
	velocity_control_effort_yaw = velocity_control_effort_yaw_msg.data;
}


void positionTrackEnableCallback(const std_msgs::Bool& position_track_enable_msg)
{
  position_track_enabled = position_track_enable_msg.data;
}

void landingConditionMetCallback(const std_msgs::Bool& landing_condition_met_msg)
{
	landing_condition_met = landing_condition_met_msg.data;
}
int main(int argc, char **argv)
{
	ros::init(argc, argv, "position_track_controller");
	ros::NodeHandle nh;

	velocity_control_x_sub = nh.subscribe("/teamhku/position_track/velocity_control_effort_x", 10, velocityControlEffortXCallback);
	velocity_control_y_sub = nh.subscribe("/teamhku/position_track/velocity_control_effort_y", 10, velocityControlEffortYCallback);
	velocity_control_yaw_sub = nh.subscribe("/teamhku/position_track/velocity_control_effort_yaw", 10, velocityControlEffortYawCallback);
	position_track_enable_sub = nh.subscribe("/teamhku/position_track/position_track_enable", 1, positionTrackEnableCallback );
	landing_condition_met_sub = nh.subscribe("/teamhku/position_track/landing_condition_met", 1, landingConditionMetCallback );   
	
	velocity_control_service = nh.serviceClient<dji_sdk::VelocityControl>("dji_sdk/velocity_control");
	sdk_permission_control_service = nh.serviceClient<dji_sdk::SDKPermissionControl>("dji_sdk/sdk_permission_control");

	dji_sdk::VelocityControl velocity_control;

	int frame = 0;
	double yaw_rate = 0;

	dji_sdk::SDKPermissionControl sdk_permission_control;
	sdk_permission_control.request.control_enable = 1;
	bool control_requested = false;

	while(!(sdk_permission_control_service.call(sdk_permission_control) && sdk_permission_control.response.result))
	{
		ROS_ERROR("request control failed!");
	}

	while(ros::ok())
	{
		ros::spinOnce();

		if(position_track_enabled)
		{
			if(landing_condition_met)
			{
				velocity_control.request.frame = frame;
				velocity_control.request.vx = velocity_control_effort_x;
				velocity_control.request.vy = velocity_control_effort_y;
				velocity_control.request.vz = descending_speed;
				velocity_control.request.yawRate = velocity_control_effort_yaw;

				if(!(velocity_control_service.call(velocity_control) && velocity_control.response.result))
				{
					ROS_ERROR("velocity control failed!");
				}
			}
			else
			{
				velocity_control.request.frame = frame;
				velocity_control.request.vx = velocity_control_effort_x;
				velocity_control.request.vy = velocity_control_effort_y;
				velocity_control.request.vz = 0;
				velocity_control.request.yawRate = velocity_control_effort_yaw;

				if(!(velocity_control_service.call(velocity_control) && velocity_control.response.result))
				{
					ROS_ERROR("velocity control failed!");
				}
			}
			
		}
		else
		{
			continue;
		}	
	}

	sdk_permission_control.request.control_enable = 0;
	sdk_permission_control_service.call(sdk_permission_control);

}