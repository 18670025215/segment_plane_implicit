#ifndef _MEANSHIFT_H_
#define _MEANSHIFT_H_

#include "common.h"
#include "typesdef.h"

class MeanShift
{
public:
	static const float NEAREST_ZERO;
	static const float C;  //!��˹�˺����еĳ���

	MeanShift() : m_size(0), R(0.0f){}

	 /** \brief �����������
	   * \param[in]  pPntCloud �������
	   */
	bool setInputCloud(const PointCloud<PointXYZ>::Ptr pPntCloud);


	 /** \brief ��ȡ�����ĵ���  */
	VecVecPoint & getOutputCloud()
	{
		return vv_pnt;
	}

	/** \brief ����K�����㷨�������뾶
	   * \param[in]  radius �뾶
	   */
	bool setKNNRadius(const float radius)
	{
		R = radius;
        return true;
	}

	 /** \brief ִ��MeanShift�����㷨	   */
	bool process();

	 /** \brief ��������ĵ����ļ�
	   * \param[in] dir_name �����Ŀ¼
	   * \param[in] prex_name �ļ���ǰ׺
	   */
	bool SaveFile(const char *dir_name, const char *prex_name);

private:
	size_t m_size;  //!Ҫ�����ĸ���
	PointCloud<PointXYZ>::Ptr mp_pointcloud;  //!PCL��ʽ�ĵ��ƣ���Ҫ��Ϊ��ʹ��FLANN��KD��
	VecPoint mv_pntcld;  //!����
	VecPoint mv_local_mode;  //!ÿ����ľֲ�ģʽ
	VecVecPoint vv_pnt;  //!�����ĵ���

	float R;  //!K�����㷨ʱ��Ӧ�������뾶

	 /** \brief ��ÿ����ִ��MeanShift
	   * \param[in]  in_pnt ����ĵ�
	   * \param[out] out_pnt ����ĵ�
	   */
	inline bool execMeanShiftEachPoint(const PointXYZ &in_pnt, Point &out_pnt);

	 /** \brief ��������ͬ�ֲ�ģʽ�ĵ��Ϊһ��
	   * \param[in] v_localmode �������Ӧ�ľֲ�ģʽ
	   * \param[out] vv_pnt �鲢��ĵ�
	   */
	bool mergeSameLocalModePoint(const VecPoint &v_localmode, VecVecPoint &vv_pnt);

	inline float gauss(float x)
	{
		return C * sqrt(x) * exp(-0.5 * x);
	}

};

#endif
