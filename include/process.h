#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "common.h"
#include "typesdef.h"

typedef std::vector<PointCloudPlane>::size_type v_pcp_size_type;

typedef std::vector<pcl::PointIndices> VecPointIndices;

 /** \brief �����ư�ǽ����о���
   * \param[in] pointcloud �������
   * \param[out] vv_pnt �����ĵ���
   */
bool cluster_points(const pcl::PointCloud<pcl::PointXYZ>::Ptr pointcloud, VecVecPoint &vv_pnt);

/** \brief 	��������ָ��ƽ��
   * \param[in] pointcloud  ���ָ�ĵ��� 
   * \param[out] v_planecoeff  �洢�ָ���ÿ��ƽ���ϵ��
   * \param[out] v_vec_point  �洢�ָ���ÿ��ƽ���������ĵ�
   * \param[out] is_savefile  �Ƿ񱣴�ָ��ĵ����ļ�
   */
bool segment_plane(const pcl::PointCloud<pcl::PointXYZ>::Ptr pointcloud, 
	VecPointIndices &v_pointindices, VecPlaneCoeff &v_planecoeff, std::vector<VecPoint> &v_vec_point, bool is_savefile);

/** \brief 	��������ָ��ƽ��
   * \param[in] pointcloud  ���ָ�ĵ��� 
   * \param[out] v_planecoeff  �洢�ָ���ÿ��ƽ���ϵ��
   * \param[out] v_vec_point  �洢�ָ���ÿ��ƽ���������ĵ�
   * \param[out] is_savefile  �Ƿ񱣴�ָ��ĵ����ļ�
   */
bool segment_plane(const pcl::PointCloud<pcl::PointXYZ>::Ptr pointcloud, 
	VecPointIndices &v_pointindices, VecPlaneCoeff &v_planecoeff, std::vector<VecPoint> &v_vec_point, bool is_savefile, size_t pln_index);

 /** \brief  ����һ��Vector������洢PointCloudPlane
   * \param[in]  v_planecoeff  ƽ�淽��ϵ��
   * \param[in]  v_vec_point  ƽ���������ĵ���
   * \param[out]  v_pcp  �洢PointCloudPlane������
   */
bool build_vec_pointcloudplane(const std::vector<VecPoint> &v_vec_point, const VecPlaneCoeff &v_planecoeff, VecPointCloudPlane &v_pcp);

 /** \brief  ��ƽ���Ҽ���̵�ƽ����ൽһ��
   * \param[in]  v_pntcldpln_before ����֮ǰ��ƽ��
   * \param[out]  v_vec_pntcldpln_after ����֮���ƽ����
   */
bool cluster_parallel_plane(const VecPointCloudPlane &v_pntcldpln_before, std::vector<VecPointCloudPlane> &v_vec_pntcldpln_after);

 /** \brief  �����洢��ƽ�е�ƽ���壬����ƽ��ƽ�����е�ƽ��鲢
   * \param[in]  v_vec_pntcldpln �洢��ƽ��ƽ����
   * \param[out]  v_pntcldpln �鲢֮��ķ�ƽ��ƽ����
   */
bool traverse_merge_planes(std::vector<VecPointCloudPlane> & v_vec_pntcldpln, VecPointCloudPlane & v_pntcldpln);

 /** \brief  ��PointCloudPlaneת��ΪPointCloud
   * \param[in]  in_pcp �����PointCloudPlane
   * \param[out]  cld �����PointCloud
   */
bool pointcloudplane2pointcloud(const PointCloudPlane & in_pcp, PointCloud<PointXYZ>::Ptr cld);

 /** \brief  ����ȷ����ƽ��Ķ���
 * \param[in]  pntcld_original ԭʼ����
   * \param[in]  v_pntcldpln PointCloudPlane��ƽ��
   * \param[out]  v_rect ��ƽ������Ӧ�ľ���
   */
bool traverse_determin_planes_verticles(const PointCloud<PointXYZ>::Ptr pntcld_original, /*const*/ VecPointCloudPlane & v_pntcldpln, VecRect & v_rect);

 /** \brief  �Ӿ���3������ȷ��һ������ƽ��
   * \param[in]  rect �����Rect���������ε�3������
   * \param[out]  cld �õ��ĵ���PointCloud
   */
bool determin_plane_from_rect(Rect & rect, PointCloud<PointXYZ>::Ptr cld);

 /** \brief  �Ӿ���4������ȷ��һ������ƽ�棬ֻ��������
   * \param[in]  rect �����Rect���������ε�4������
   * \param[out]  cld �õ��ĵ���PointCloud
   */
bool determin_plane_from_rect_only_contour(Rect & rect, PointCloud<PointXYZ>::Ptr cld);

#endif