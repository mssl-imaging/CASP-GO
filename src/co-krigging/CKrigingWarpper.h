#ifndef CKRIGINGWARPPER_H
#define CKRIGINGWARPPER_H

#include "CDTMParam.h"
#include "CProcBlock.h"




////////////////////////////////////////////
// this is for Kriging
////////////////////////////////////////////
#include <GsTL/kriging/kriging_weights.h>
#include <GsTL/kriging/SK_constraints.h>
#include <GsTL/kriging/OK_constraints.h>
#include <GsTL/kriging/KT_constraints.h>
#include <GsTL/kriging/linear_combination.h>
#include <GsTL/kriging/cokriging_weights.h>

#include <GsTL/cdf_estimator/gaussian_cdf_Kestimator.h>
#include <GsTL/cdf/gaussian_cdf.h>

#include <GsTL/geometry/covariance.h>

#include <GsTL/kriging/kriging_constraints.h>
#include <GsTL/kriging/kriging_combiner.h>

class PointK{
public:
  typedef double coordinate_type;

  PointK(){X=0;Y=0;}
  PointK(int x, int y): X(x), Y(y) {}
  int& operator[](int i){
    if(i==1) return X;
    else     return Y;
  }

  const int& operator[](int i) const {
    if(i==1) return X;
    else     return Y;
  }

  int X;
  int Y;
};

//-------------------------

class Node{
public:
  typedef double property_type;
  typedef PointK location_type;

  Node():pval_(0) {loc_.X=0; loc_.Y=0;}
  Node(int x, int y, double pval){loc_.X=x; loc_.Y=y;pval_=pval;}
  inline PointK location() const { return loc_;}
  inline double& property_value(){return pval_;}
  inline const double& property_value() const {return pval_;}
  inline void setData(const Point3f ptXYZ){loc_.X=ptXYZ.x; loc_.Y=ptXYZ.y;pval_=ptXYZ.z;}
private:
  PointK loc_;
  double pval_;
};

//std::ostream& operator << (std::ostream& os, Node& N){
//  std::cout << N.location().X << " "
//            << N.location().Y << " ";

//  return os;
//};


//---------------------------
class covariance{
public:
  inline double operator()(const PointK& P1,const PointK& P2) const {
    double h=sqrt((P1.X-P2.X,2)*(P1.X-P2.X,2)+(P1.Y-P2.Y,2)*(P1.Y-P2.Y,2));
    double a=10;
    double c=1.0;
    return c * exp(-3*h/a);
  }
};


//struct My_covariance_set {
//  inline double operator()(int i, int j,
//                           const Point& P1,const Point& P2) const {
//    double h=sqrt(pow(P1.X-P2.X,2)+pow(P1.Y-P2.Y,2));
//    double a=10;
//    return exp(-3*h/a);
//  }
//};

//-----------------------------

class neighborhood{
public:
  typedef std::vector<Node>::iterator iterator;
  typedef std::vector<Node>::const_iterator const_iterator;

  void add_node(Node n){neigh_.push_back(n);}
  iterator begin(){return neigh_.begin();}
  iterator end(){return neigh_.end();}
  const_iterator begin() const {return neigh_.begin();}
  const_iterator end() const {return neigh_.end();}
  unsigned int size() const {return neigh_.size();}
  bool is_empty() const {return neigh_.empty();}

private:
  void find_neighbors(Point& u);

  std::vector<Node> neigh_;
};
////////////////////////////////////////////

class CProjXY{
public:
    vector<Point3f> m_vecptXYZ;
    vector<Point3i> m_vecptRGB;

//private:
    double getDistance(const Point2f& ptA, const int nIdx){
        Point2f ptTemp;
        ptTemp.x = m_vecptXYZ[nIdx].x;
        ptTemp.y = m_vecptXYZ[nIdx].y;
        return norm(ptA - ptTemp);
    }
};
////////////////////////////////////////////

class CKrigingWarpper : public CProcBlock
{
public:
    CKrigingWarpper();
    CKrigingWarpper(CDTMParam paramDTM);
    ~CKrigingWarpper();
    void performInterpolation(const string strPly, const string strOut);

protected:
    void readData(const string strPly);
    void prepareProjectionMap();
    void interpolation();
    void clear2DBuffer();

private:
    void getNeighbourData(const Point2i& ptCentre, vector<Point3f>& vecNeiXYZ, vector<Point3i>& vecNeiCol);

protected:
    // inputs
    CDTMParam m_paramDTM;

    // outputs
    Mat m_matDTM;
    Mat m_matImg;
    Mat m_matVarDTM;
    Mat m_matVarImg;

private:
    // temp
    vector<Point3f> m_vecptXYZ; // input 3d points from ply
    vector<Point3i> m_vecptCol; // input colour data from ply
    CProjXY ** m_ppProjData;    // needed to estimate the neighbourhood of a point in Kriging
};

#endif // CKRIGINGWARPPER_H
