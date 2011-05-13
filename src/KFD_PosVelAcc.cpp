#include "KFD_PosVelAcc.hpp"
#include <Eigen/LU> 
#include <math.h>
#include "FaultDetection.hpp"

using namespace pose_ekf;


/** CHECK */ 
KFD_PosVelAcc::KFD_PosVelAcc() 
{
    filter =  new KalmanFilter::KF<StatePosVelAcc::SIZE>();
    position_world = Eigen::Vector3d::Zero();
    velocity_inertial = Eigen::Vector3d::Zero();
    
    std::cout << " Instanciando KFD_PosVelAcc " << std::endl; 
}

KFD_PosVelAcc::~KFD_PosVelAcc()
{
    delete filter; 
    
}

void KFD_PosVelAcc::predict(Eigen::Vector3d acc_intertial, double dt, Eigen::Matrix<double, StatePosVelAcc::SIZE, StatePosVelAcc::SIZE> process_noise)
{
 	  
	  /** Inertial Navigation */ 
	  //gravity correction 
	  Eigen::Vector3d gravity_inertial = Eigen::Vector3d::Zero(); 
	  gravity_inertial.z() = 9.871; 
	  
	  //gravity correction 
	  acc_intertial = acc_intertial - gravity_inertial; 
	  
	  velocity_inertial = velocity_inertial + acc_intertial * dt; 
	  //inertial navigation 
	  position_world = position_world 
	      + velocity_inertial * dt;
	    
	  
	  /** Kalman Filter */ 
	  
	  //sets the transition matrix 
	  Eigen::Matrix<double, StatePosVelAcc::SIZE, StatePosVelAcc::SIZE> F;
	  F.setZero();
	  
	  F.block<3,3>(0,3) =  Eigen::Matrix3d::Identity(); 
	  F.block<3,3>(3,6) =  Eigen::Matrix3d::Identity() ; 
	  
	  
	 //std::cout << "F \n" << F << std::endl; 
	/* std::cout 
	      << "0 0 0 1 0 0 0 0 0 \n" 
	      << "0 0 0 0 1 0 0 0 0 \n"  
	      << "0 0 0 0 0 1 0 0 0 \n"  
	      << "0 0 0 0 0 0 1 0 0 \n" 
	      << "0 0 0 0 0 0 0 1 0 \n"  
	      << "0 0 0 0 0 0 0 0 1 \n"  
	      << "0 0 0 0 0 0 0 0 0 \n" 
	      << "0 0 0 0 0 0 0 0 0 \n" 
	      << "0 0 0 0 0 0 0 0 0 \n" 
	      <<std::endl;*/
	  
	  //updates the Kalman Filter 
	  filter->predictionDiscrete( F, process_noise, dt); 

	  //get the updated values 
	  x.vector()=filter->x; 
	  
	 
	  
}

bool KFD_PosVelAcc::positionObservation(Eigen::Vector3d position, Eigen::Matrix3d covariance, float reject_position_threshol)
{
  
      Eigen::Vector3d  position_diference = position_world - position; 

      Eigen::Matrix<double, _POS_MEASUREMENT_SIZE, StatePosVelAcc::SIZE> H; 
      H.setZero(); 
      H.block<3,3>(0,0) = Eigen::Matrix3d::Identity(); 
      
      //position_world = position;
      bool reject =  filter->correctionChiSquare<_POS_MEASUREMENT_SIZE,_POS_DEGREE_OF_FREEDOM>( position_diference,  covariance,H, reject_position_threshol );
      
      x.vector() = filter->x;
      
      correct_state();
      
      return reject; 
      
}

bool KFD_PosVelAcc::velocityObservation(Eigen::Vector3d velocity_body, Eigen::Matrix3d covariance, float reject_velocity_threshol)
{
  
      Eigen::Vector3d  velocity_diference = velocity_inertial - velocity_body; 

      Eigen::Matrix<double, _VEL_MEASUREMENT_SIZE, StatePosVelAcc::SIZE> H; 
      H.setZero(); 
      H.block<3,3>(3,3) = Eigen::Matrix3d::Identity(); 
      
      //position_world = position;
      bool reject =  filter->correctionChiSquare<_VEL_MEASUREMENT_SIZE,_VEL_DEGREE_OF_FREEDOM>( velocity_diference,  covariance, H, reject_velocity_threshol );
      
      x.vector() = filter->x;
      
      correct_state();
      
      return reject; 
      
}

void KFD_PosVelAcc::setPosition( Eigen::Vector3d position, Eigen::Matrix3d covariance )
{
  
      //set the inertial navigation for the initial position 
      position_world = position; 
      
      //The initial covariance in position 
      filter->P.block<3,3>(0,0) = covariance ;
      
      x.pos_world() = Eigen::Vector3d::Zero(); 
      filter->x = x.vector();

}

void KFD_PosVelAcc::correct_state()
{

    position_world = position_world  -  x.pos_world(); 
    
    velocity_inertial = velocity_inertial -  x.vel_inertial(); 
    
    //once the state is corrected I can set the erro estimate to 0 
    x.pos_world() = Eigen::Vector3d::Zero(); 
    x.vel_inertial() = Eigen::Vector3d::Zero(); 
    
    filter->x = x.vector(); 
    
}

void KFD_PosVelAcc::copyState ( const KFD_PosVelAcc& kfd )
{
  
    x = kfd.x; 
    filter->x = kfd.filter->x;
    filter->P = kfd.filter->P;
    position_world = kfd.position_world; 
    velocity_inertial = kfd.velocity_inertial;
    
}


Eigen::Vector3d KFD_PosVelAcc::getPosition()
{
    return (position_world - x.pos_world()); 
}
	
Eigen::Vector3d KFD_PosVelAcc::getVelocity()
{
    return (velocity_inertial - x.vel_inertial()); 
}

Eigen::Matrix3d KFD_PosVelAcc::getPositionCovariance()
{
    return filter->P.block<3,3>(0,0);
}

Eigen::Matrix3d KFD_PosVelAcc::getVelocityCovariance()
{
    return filter->P.block<3,3>(3,3);
}



/** configurarion hook */ 
void KFD_PosVelAcc::init(const Eigen::Matrix<double, StatePosVelAcc::SIZE, StatePosVelAcc::SIZE> &P, const Eigen::Matrix<double,StatePosVelAcc::SIZE,1> &x)
{
    Q.setZero(); 
    this->x.vector() = x; 
    filter->x = x; 
    filter->P = P; 

}