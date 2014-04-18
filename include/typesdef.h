#ifndef _TYPESDEF_H_
#define _TYPESDEF_H_

#include <math.h>
#include <iostream>
#include <vector>
/*
 *	vector�洢PointXYZ��������Ҫ���������std::vector<PointT, Eigen::aligned_allocator<PointT> > points;
 */
const float NEAREST_ZERO = 0.001;
const float EPSILON = 1.5;
const float ANGLE_THRESHOLD = (M_PI / 180 * 12.5);  //!ƽ���ļн�С�ڸ���ֵ���ɽ��ƿ���ƽ��
const float PLN2PLN_THRESHOLD = 1.4;//;1.8;//1.0;  //!������ƽ��ƽ���ľ���С��PLN2PLN_THRESHOLD���ͽ���ƽ���ں�
const float CENTER2CENTER_THRESHOLD = 6.0;  //!������ƽ��ƽ������ļ�ľ���С��CENTER2CENTER_THRESHOLD�� �ͽ���ƽ���ں�
const float PNT2PLANE_THRESHOLD = 1.8;  //!�㵽ƽ������ֵ��������С�ڸ���ֵ�򽫸õ㿴����ƽ���һ����

const float SEG_DIST_THRESHOLD = 0.15;//0.15; //0.06; //!�ָ�ƽ��ʱ�ľ�����ֵ
const int MAX_ITERS = 220;//100;//220;  //!ƽ��ָ�ʱ�������һ�����㷨�еĵ�������

/*
 *	����һ��ƽ��ϵ���ṹ�壬ƽ�淽��һ����Ա�ʾΪax+by+cz+d = 0
 */
typedef struct _PlaneCoeff
{
	float a;
	float b;
	float c;
	float d;
	
	_PlaneCoeff()
	{
		a = 0;
		b = 0;
		c = 0;
		d = 0;
	}
	_PlaneCoeff(const float aa, const float bb, const float cc, const float dd)
	{
		a = aa;
		b = bb;
		c = cc;
		d = dd;
	}

	_PlaneCoeff(const _PlaneCoeff &coeff)
	{
		a = coeff.a;
		b = coeff.b;
		c = coeff.c;
		d = coeff.d;
	}
 
	_PlaneCoeff & operator=(const _PlaneCoeff &coeff)
	{
		a = coeff.a;
		b = coeff.b;
		c = coeff.c;
		d = coeff.d;
		return *this;
	}
}PlaneCoeff;

typedef struct _Point
{
	float x;
	float y;
	float z;
	_Point()
	{

	}
	_Point(float aa, float bb, float cc)
	{
		x = aa;
		y = bb;
		z = cc;
	}
	_Point(const _Point &pnt)
	{
		x = pnt.x;
		y = pnt.y;
		z = pnt.z;
	}
    _Point & operator=(const _Point &pnt)
    {
        x = pnt.x;
		y = pnt.y;
		z = pnt.z;
		return *this;
    }

	friend bool operator==(const _Point &pnt1, const _Point &pnt2)
	{
		float diff_x = pnt1.x - pnt2.x;
		float diff_y = pnt1.y - pnt2.y;
		float diff_z = pnt1.z - pnt2.z;
		
		return (-NEAREST_ZERO < diff_x && diff_x < NEAREST_ZERO)
			&& (-NEAREST_ZERO < diff_y && diff_y < NEAREST_ZERO)
			&& (-NEAREST_ZERO < diff_z && diff_z < NEAREST_ZERO);
	}

   friend std::ostream & operator<<(std::ostream & os, const _Point &pnt)
   {
        os<<pnt.x<<", "<<pnt.y<<", "<<pnt.z<<std::endl;
        return os;
   }

	inline float getVecLength()
	{
		return sqrt(x * x + y * y + z * z);
	}

	friend float getPointsDist(const _Point & pnt1, const _Point & pnt2)
	{
		float diff_x = pnt1.x - pnt2.x, diff_y = pnt1.y - pnt2.y, diff_z = pnt1.z - pnt2.z;

		return sqrt(diff_x * diff_x + diff_y * diff_y + diff_z * diff_z);
	}
}Point;

typedef struct Vector : public Point
{
	Vector(){}
	Vector(float aa, float bb, float cc) : Point(aa, bb, cc)
	{
	}

    Vector & normalize()
	{
		float vec_len = getVecLength();
		x /= vec_len;
		y /= vec_len;
		z /= vec_len;

		return *this;
	}

	bool isVertical(const Vector &vec)
	{
		float dot_prod = fabsf(x * vec.x + y * vec.y + z * vec.z);
		return dot_prod <= sinf(M_PI / 180 * 16.5);
	}

}Vector;

Vector makeVector(const _Point & pnt1, const _Point & pnt2);

typedef std::vector<PlaneCoeff> VecPlaneCoeff;
typedef std::vector<Point> VecPoint;
typedef std::vector<VecPoint> VecVecPoint;

typedef struct _PointCloudPlane
{
	size_t m_indice;  //!����ƽ����������
    VecPoint mv_pointcloud;  //!����ƽ������ĵ�
	PlaneCoeff m_coeff;  //!����ƽ���ϵ��

    _PointCloudPlane()
    {
    }

	_PointCloudPlane(size_t indice, const VecPoint &v_pointcloud, const PlaneCoeff & coeff)
	{
		m_indice = indice;
		mv_pointcloud = v_pointcloud;
		m_coeff = coeff;
	}
}PointCloudPlane;

typedef struct _Rect
{
	size_t m_indice; //!����ƽ����������
	Point m_verticle1;
	Point m_verticle2;
	Point m_verticle3;
	Point m_verticle4;
	_Rect(){}
	_Rect(size_t &indice, Point &verticle1, Point &verticle2, Point &verticle3, Point &verticle4)
	{
		m_indice = indice;
		m_verticle1 = verticle1;
		m_verticle2 = verticle2;
		m_verticle3 = verticle3;
		m_verticle4 = verticle4;
	}
}Rect;

typedef std::vector<PointCloudPlane> VecPointCloudPlane;
typedef std::vector<Rect> VecRect;

#endif    //_TYPESDEF_H_
