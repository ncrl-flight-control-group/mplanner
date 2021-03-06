#include <ros/ros.h>
#include "../lib/mavlink_v2/ncrl_mavlink/mavlink.h"
#include "ncrl_mavlink.h"
#include "pose.hpp"
#include "ros_thread.hpp"

uav_pose_t uav_pose;

bool received_traj_ack = false;
uint8_t traj_ack_val;

void mavlink_attitude_quaternion_handler(mavlink_message_t *received_msg)
{
	//ROS_INFO("[mavlink]received attitude message.");

	mavlink_attitude_quaternion_t attitude_quaternion;
	mavlink_msg_attitude_quaternion_decode(received_msg, &attitude_quaternion);

	uav_pose.q[0] = attitude_quaternion.q1;
	uav_pose.q[1] = attitude_quaternion.q2;
	uav_pose.q[2] = attitude_quaternion.q3;
	uav_pose.q[3] = attitude_quaternion.q4;
}

void mavlink_local_position_ned_handler(mavlink_message_t *received_msg)
{
	//ROS_INFO("[mavlink]received local position message.");

	mavlink_local_position_ned_t position_ned;
	mavlink_msg_local_position_ned_decode(received_msg, &position_ned);

	uav_pose.pos_ned[0] = position_ned.x;
	uav_pose.pos_ned[1] = position_ned.y;
	uav_pose.pos_ned[2] = position_ned.z;
	uav_pose.vel_ned[0] = position_ned.vx;
	uav_pose.vel_ned[1] = position_ned.vy;
	uav_pose.vel_ned[2] = position_ned.vz;
}

void mavlink_polynomial_trajectory_ack_handler(mavlink_message_t *received_msg)
{
	mavlink_polynomial_trajectory_ack_t traj_ack;
	mavlink_msg_polynomial_trajectory_ack_decode(received_msg, &traj_ack);

	traj_ack_val = traj_ack.ack_val;

	received_traj_ack = true;
}

void mavlink_polynomial_trajectory_position_debug_handler(mavlink_message_t *received_msg)
{
	float curr_pos[3] = {0.0f};
	float des_pos[3] = {0.0f};

	mavlink_polynomial_trajectory_position_debug_t pos_debug_data;
	mavlink_msg_polynomial_trajectory_position_debug_decode(
		received_msg, &pos_debug_data);

	curr_pos[0] = pos_debug_data.x;
	curr_pos[1] = pos_debug_data.y;
	curr_pos[2] = pos_debug_data.z;
	des_pos[0] = pos_debug_data.x_d;
	des_pos[1] = pos_debug_data.y_d;
	des_pos[2] = pos_debug_data.z_d;

	//XXX
	uav_pose.pos_ned[0] = pos_debug_data.x;
	uav_pose.pos_ned[1] = pos_debug_data.y;
	uav_pose.pos_ned[2] = pos_debug_data.z;

	update_uav_trajectory_position_debug(curr_pos, des_pos);
}

void mavlink_polynomial_trajectory_velocity_debug_handler(mavlink_message_t *received_msg)
{
	float curr_vel[3] = {0.0f};
	float des_vel[3] = {0.0f};

	mavlink_polynomial_trajectory_velocity_debug_t vel_debug_data;
	mavlink_msg_polynomial_trajectory_velocity_debug_decode(
		received_msg, &vel_debug_data);

	curr_vel[0] = vel_debug_data.vx;
	curr_vel[1] = vel_debug_data.vy;
	curr_vel[2] = vel_debug_data.vz;
	des_vel[0] = vel_debug_data.vx_d;
	des_vel[1] = vel_debug_data.vy_d;
	des_vel[2] = vel_debug_data.vz_d;

	update_uav_trajectory_velocity_debug(curr_vel, des_vel);
}

void mavlink_polynomial_trajectory_acceleration_debug_handler(mavlink_message_t *received_msg)
{
	float accel_ff[3] = {0.0f};

	mavlink_polynomial_trajectory_acceleration_debug_t accel_debug_data;
	mavlink_msg_polynomial_trajectory_acceleration_debug_decode(
		received_msg, &accel_debug_data);

	accel_ff[0] = accel_debug_data.ax_ff;
	accel_ff[1] = accel_debug_data.ay_ff;
	accel_ff[2] = accel_debug_data.az_ff;

	update_uav_trajectory_acceleration_debug(accel_ff);
}

bool wait_mavlink_polynomial_trajectory_ack(uint8_t *ack_val)
{
	double start_time = ros::Time::now().toSec();

	//TODO: replace with spinlock?
	while(received_traj_ack == false) {
		double current_time = ros::Time::now().toSec();
		double elapsed_time = current_time - start_time;

		/* one seconds timeout */
		if(elapsed_time >= 1.0f) {
			return false;
		}
	}

	received_traj_ack = false;

	*ack_val = traj_ack_val;
	return true;
}

