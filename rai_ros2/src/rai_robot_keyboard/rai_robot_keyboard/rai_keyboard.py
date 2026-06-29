#!/usr/bin/env python
# coding=utf-8
# Copyright (c) 2011, Willow Garage, Inc.
# All rights reserved.
#
# Software License Agreement (BSD License 2.0)
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials provided
#    with the distribution.
#  * Neither the name of {copyright_holder} nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# Author: Darby Lim

import os
import select
import sys
import rclpy

from geometry_msgs.msg import Twist
from rclpy.qos import QoSProfile

if os.name == 'nt':
    import msvcrt
else:
    import termios
    import tty

BURGER_MAX_LIN_VEL = 0.22
BURGER_MAX_ANG_VEL = 2.84

WAFFLE_MAX_LIN_VEL = 0.26
WAFFLE_MAX_ANG_VEL = 1.82

LIN_VEL_STEP_SIZE = 0.01
ANG_VEL_STEP_SIZE = 0.1


msg = """
Control Your Mecanum Robot!
---------------------------
Moving around:
   w(↑) : forward
   a(←) : left
   s(↓) : backward
   d(→) : right
   q    : forward-left diagonal
   e    : forward-right diagonal
   z    : backward-left diagonal
   c    : backward-right diagonal

Rotate in place:
   x    : rotate clockwise
   Ctrl+x : rotate counter-clockwise

Speed adjustment:
   u/i : increase/decrease linear speed
   o/p : increase/decrease angular speed

space key : force stop
b : switch to OmniMode/CommonMode
CTRL-C to quit
"""
e = """
Communications Failed
"""
moveBindings = {
        'w':( 1, 0, 0),   # forward
        'a':( 0, 1, 0),   # left
        's':(-1, 0, 0),   # backward
        'd':( 0,-1, 0),   # right
        'q':( 1, 1, 0),   # forward-left diagonal
        'e':( 1,-1, 0),   # forward-right diagonal
        'z':(-1, 1, 0),   # backward-left diagonal
        'c':(-1,-1, 0),   # backward-right diagonal
        'x':( 0, 0,-1),   # rotate clockwise
        '\x18':( 0, 0, 1), # Ctrl+x: rotate counter-clockwise
           }

speedBindings={
        'u':(1.1, 1),     # increase linear speed
        'i':(0.9, 1),     # decrease linear speed
        'o':(1, 1.1),     # increase angular speed
        'p':(1, 0.9),     # decrease angular speed
          }

speed = 0.2
turn  = 1.0

def get_key(settings):
    if os.name == 'nt':
        return msvcrt.getch().decode('utf-8')
    tty.setraw(sys.stdin.fileno())
    rlist, _, _ = select.select([sys.stdin], [], [], 0.1)
    if rlist:
        key = sys.stdin.read(1)
    else:
        key = ''

    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, settings)
    return key

def print_vels(speed, turn):
    print('currently:\tspeed {0}\t turn {1} '.format(
        speed,
        turn))

def main():
    settings = None
    if os.name != 'nt':
        settings = termios.tcgetattr(sys.stdin)
    rclpy.init()

    qos = QoSProfile(depth=10)
    node = rclpy.create_node('rai_keyboard')
    pub = node.create_publisher(Twist, 'cmd_vel', qos)
    speed = 0.2
    turn  = 1.0
    x      = 0.0
    y      = 0.0
    th     = 0.0
    count  = 0.0
    target_speed_x = 0.0
    target_speed_y = 0.0
    target_turn  = 0.0
    control_speed_x = 0.0
    control_speed_y = 0.0
    control_turn  = 0.0
    Omni = 1
    try:
        print(msg)
        print(print_vels(speed, turn))
        while(1):
            key = get_key(settings)
            if key=='b':
                print("Mecanum robot - always in OmniMode")
            
            if key in moveBindings.keys():
                x  = moveBindings[key][0]
                y  = moveBindings[key][1]
                th = moveBindings[key][2]
                count = 0

            elif key in speedBindings.keys():
                speed = speed * speedBindings[key][0]
                turn  = turn  * speedBindings[key][1]
                count = 0
                print(print_vels(speed,turn))

            elif key == ' ' or key == 'k' :
                x  = 0.0
                y  = 0.0
                th = 0.0
                control_speed_x = 0.0
                control_speed_y = 0.0
                control_turn  = 0.0

            else:
                count = count + 1
                if count > 4:
                    x  = 0.0
                    y  = 0.0
                    th = 0.0
                if (key == '\x03'):
                    break

            target_speed_x = speed * x
            target_speed_y = speed * y
            target_turn  = turn * th

            if target_speed_x > control_speed_x:
                control_speed_x = min( target_speed_x, control_speed_x + 0.1 )
            elif target_speed_x < control_speed_x:
                control_speed_x = max( target_speed_x, control_speed_x - 0.1 )
            else:
                control_speed_x = target_speed_x

            if target_speed_y > control_speed_y:
                control_speed_y = min( target_speed_y, control_speed_y + 0.1 )
            elif target_speed_y < control_speed_y:
                control_speed_y = max( target_speed_y, control_speed_y - 0.1 )
            else:
                control_speed_y = target_speed_y

            if target_turn > control_turn:
                control_turn = min( target_turn, control_turn + 0.5 )
            elif target_turn < control_turn:
                control_turn = max( target_turn, control_turn - 0.5 )
            else:
                control_turn = target_turn

            twist = Twist()
            twist.linear.x  = control_speed_x
            twist.linear.y  = control_speed_y
            twist.linear.z  = 0.0
            twist.angular.x = 0.0
            twist.angular.y = 0.0
            twist.angular.z = control_turn
            pub.publish(twist)

    except Exception as e:
        print(e)

    finally:
        twist = Twist()
        twist.linear.x = 0.0
        twist.linear.y = 0.0
        twist.linear.z = 0.0

        twist.angular.x = 0.0
        twist.angular.y = 0.0
        twist.angular.z = 0.0

        pub.publish(twist)

        if os.name != 'nt':
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, settings)


if __name__ == '__main__':
    main()
