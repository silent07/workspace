#include "ros/ros.h"
#include "people_msgs/PositionMeasurement.h"
#include "tf/transform_listener.h"
#include "tf/message_filter.h"
#include "message_filters/subscriber.h"
#include <visualization_msgs/Marker.h>
#include <list>
#include "geometry_msgs/Point.h"
#include <math.h>
#include<fstream>

using namespace std;
using namespace ros;
using namespace tf;

static string fixed_frame = "odom_combined";
const float time_interval = .5;
ofstream outfile;
class PeoplePositionVelocity
{
public:
string object_id_;
Stamped<Point> position_;
float v[3];
PeoplePositionVelocity(Stamped<Point> pos,string id)
  {
    position_=pos;
    v[0]=100;v[1]=100;v[2]=0;
    object_id_=id;
  }

}; 

class TrajGenerator
{

public:
NodeHandle nh_;
TransformListener tfl_;
ros::Publisher markers_pub_;
list<PeoplePositionVelocity*> people_position_velocity_;

message_filters::Subscriber<people_msgs::PositionMeasurement> people_sub_;
tf::MessageFilter<people_msgs::PositionMeasurement> people_notifier_;

TrajGenerator(ros::NodeHandle nh) :
    nh_(nh), 
people_sub_(nh_,"people_tracker_measurements",10),
people_notifier_(people_sub_,tfl_,fixed_frame,10)
{
people_notifier_.registerCallback(boost::bind(&TrajGenerator::peopleCallback, this, _1));
      people_notifier_.setTolerance(ros::Duration(0.01));

markers_pub_ = nh_.advertise<visualization_msgs::Marker>("visualization_marker", 20);
}

~TrajGenerator()
{
}

void peopleCallback(const people_msgs::PositionMeasurement::ConstPtr& people_meas)
{
list<PeoplePositionVelocity*>::iterator begin = people_position_velocity_.begin();
list<PeoplePositionVelocity*>::iterator end = people_position_velocity_.end();
list<PeoplePositionVelocity*>::iterator it1, it2;
bool found=0;
int i=0;
    for (it1=begin;it1!=end;it1++,i++)
    {
       if((*it1)->object_id_ == people_meas->object_id)
       {
        found=1;
        //if time stamp differ by time interval sec
        if (people_meas->header.stamp.toSec() - (*it1)->position_.stamp_.toSec() > time_interval)
        {
         Point pt;
         pointMsgToTF(people_meas->pos, pt);
	 (*it1)->v[0]=(pt[0]-(*it1)->position_[0])/( people_meas->header.stamp.toSec() - (*it1)->position_.stamp_.toSec() );
	 (*it1)->v[1]=(pt[1]-(*it1)->position_[1])/( people_meas->header.stamp.toSec() - (*it1)->position_.stamp_.toSec() );
	//it1->position=*people_meas;	
         (*it1)->position_[0] = pt[0];
         (*it1)->position_[1] = pt[1];
         (*it1)->position_[2] = pt[2];
	 (*it1)->position_.stamp_=people_meas->header.stamp;
        //publish velocity marker
	    double theta = atan2((*it1)->v[1],(*it1)->v[0]);
            visualization_msgs::Marker m;
            m.header.stamp = (*it1)->position_.stamp_;
            m.header.frame_id = fixed_frame;
            m.ns = "VELOCITY";
            m.id = i;
            m.type = m.ARROW;
            m.pose.position.x = (*it1)->position_[0];
            m.pose.position.y = (*it1)->position_[1];
            m.pose.position.z = 0;
 	    m.pose.orientation.x = 0.0;
   	    m.pose.orientation.y = 0.0;
  	    m.pose.orientation.z = sin(theta/2);
  	    m.pose.orientation.w = cos(theta/2);
	    double scale = sqrt( (*it1)->v[0] * (*it1)->v[0] + (*it1)->v[1] * (*it1)->v[1] );
	    //float scale = 2;
            m.scale.x = scale;
            m.scale.y = scale;
            m.scale.z = scale;
            m.color.a = 1;
            m.color.g = 1;
            m.lifetime = ros::Duration(0.5);
	    outfile<<scale<<"   "<<(*it1)->object_id_<<"\n";
            markers_pub_.publish(m);	 
	}
        break;
       }
    }
    if (!found)
    {
      Point pt;
      pointMsgToTF(people_meas->pos, pt);
      Stamped <Point> pos (pt,people_meas->header.stamp,people_meas->header.frame_id);
      list<PeoplePositionVelocity*>::iterator new_saved = people_position_velocity_.insert(people_position_velocity_.end(), new PeoplePositionVelocity(pos,people_meas->object_id));
    }
}

};

int main(int argc, char **argv)
{  
  outfile.open("/home/siddharth/output.txt");
  ros::init(argc, argv,"traj_generator");
  ros::NodeHandle nh;
  TrajGenerator tg(nh);
  ros::spin();
  outfile.close();
//  posoutfile.close();
  return 0;
}
