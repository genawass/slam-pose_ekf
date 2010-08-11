#ifndef __KALMAN_FILTER_TYPES__
#define __KALMAN_FILTER_TYPES__

#include <base/time.h>
#include <base/wrappers/eigen.h>

namespace pose_estimator
{ 

    struct KalmanFilterState
    {

	/** the time that the slip started */
	base::Time time;       

	/** the filter state */ 
	wrappers::Vector4 x; 
	/** the filter covariance matrix */ 
	wrappers::Matrix4 P;

	/** the icp observation */ 
	wrappers::Vector4 p_icp; 
	/** the icp observation covariance */ 
	wrappers::Matrix4 R_icp;
	
	
    };
 
}

#endif

