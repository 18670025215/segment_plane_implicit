#include "common.h"
#include "other.hpp"
#include "process.h"
#include "typesdef.h"
#include "common_operate.h"
#include <assert.h>
#include <Eigen/Dense>

extern string g_str;

 /** \brief ������Ƶķ�����
   * \param[in] pCloud ��XYZ����ĵ���
   * \param[out] normals �����Normal����
   */
static void calculate_pointnormal(const PointCloud<PointXYZ>::Ptr pCloud, PointCloud<Normal>::Ptr normals)
{
	// Create search tree
	search::KdTree<PointXYZ>::Ptr tree;
	tree.reset (new search::KdTree<PointXYZ> (false));
	tree->setInputCloud (pCloud);

	// Normal estimation
	NormalEstimation<PointXYZ, Normal> n;
	n.setInputCloud (pCloud);
	//n.setIndices (indices[B);
	n.setSearchMethod (tree);
	n.setKSearch (150);
	n.compute (*normals);
}

 /** \brief �ж������������Ƿ�ƽ��
   * \param[in] normal1 ������1
   * \param[in] normal2 ������2
   */
inline bool is_normals_parallel(const Normal &normal1, const Normal &normal2)
{
	//Eigen::Vector3f n1(normal1.normal_x, normal1.normal_y, normal1.normal_z);
	//Eigen::Vector3f n2(normal2.normal_x, normal2.normal_y, normal2.normal_z);

	//n1.normalize();
	//n2.normalize();

	float cos_theta = 0, dot_prod = 0;
	//float len_n1 = 0, len_n2 = 0;
	//dot_prod = n1.dot(n2);
	dot_prod = (normal1.normal_x * normal2.normal_x + normal1.normal_y * normal2.normal_y + normal1.normal_z * normal2.normal_z);
	/*
	 *	�����������Ѿ��ǵ�λ������
	 */
	//len_n1 = sqrt(normal1.normal_x * normal1.normal_x + normal1.normal_y * normal1.normal_y + normal1.normal_z * normal1.normal_z);
	//len_n2 = sqrt(normal2.normal_x * normal2.normal_x + normal2.normal_y * normal2.normal_y + normal2.normal_z * normal2.normal_z);
	//cos_theta = fabsf(dot_prod) / (len_n1 * len_n2);
	cos_theta = fabsf(dot_prod);

	return cos_theta >= cosf(ANGLE_THRESHOLD);
}

 /** \brief �����ư�ǽ����о���
   * \param[in] pointcloud �������
   * \param[out] vv_pnt �����ĵ���
   */
bool cluster_points(const pcl::PointCloud<pcl::PointXYZ>::Ptr pointcloud, VecVecPoint &vv_pnt)
{
	PointCloud<Normal>::Ptr normals (new PointCloud<Normal> ());

	calculate_pointnormal(pointcloud, normals);

	size_t pnt_count = pointcloud->points.size();
	std::vector<bool> v_iscluster(pnt_count, false);

	for (size_t i = 0; i < pnt_count; ++i)
	{
		for (size_t j = i + 1; j < pnt_count; ++j)
		{
			const Normal & normal1 = normals->points[i];
			const Normal & normal2 = normals->points[j];
			Point  pnt1(pointcloud->points[i].x, pointcloud->points[i].y, pointcloud->points[i].z);
			Point  pnt2(pointcloud->points[j].x, pointcloud->points[j].y, pointcloud->points[j].z);
			float dist = 0.0f;

			if (is_normals_parallel(normal1, normal2))
			{
				//�����������ɽ���ƽ��
				VecPoint v_pnt;

				if ( !v_iscluster[i] &&  !v_iscluster[j])
				{
					v_iscluster[i] = true;
					v_pnt.push_back(pnt1);

					v_iscluster[j] = true;
					v_pnt.push_back(pnt2);

					vv_pnt.push_back(v_pnt);
				}
				else if ( v_iscluster[i] &&  !v_iscluster[j])
				{
					size_t clustered_count = vv_pnt.size();
					size_t m, n;
					for (m = 0; m < clustered_count; ++m)
					{
						size_t count = vv_pnt[m].size();
						for (n = 0; n < count; ++n)
						{
							if (pnt1 == vv_pnt[m][n])
							{
								vv_pnt[m].push_back(pnt2);
								v_iscluster[j] = true;
								goto LABEL1;
							}
						}  // for n
					}  // for m
LABEL1:
					;
				}  // else if
				else if ( !v_iscluster[i] &&  v_iscluster[j])
				{
					size_t clustered_count = vv_pnt.size();
					size_t m, n;
					for (m = 0; m < clustered_count; ++m)
					{
						size_t count = vv_pnt[m].size();
						for (n = 0; n < count; ++n)
						{
							if (pnt2 == vv_pnt[m][n])
							{
								vv_pnt[m].push_back(pnt1);
								v_iscluster[i] = true;
								goto LABEL2;
							}
						}  // for n
					}  // for m
LABEL2:
					;
				}
				else
				{
					//���Ѿ����࣬�Ͳ�������
				}
			}  //  if (dist <= NEAREST_ZERO)

		}  //  for j
	}  //  for i

	for (size_t i = 0; i < pnt_count; ++i)
	{
		if (!v_iscluster[i])
		{
			Point  pnt(pointcloud->points[i].x, pointcloud->points[i].y, pointcloud->points[i].z);
			VecPoint v_pnt;

			v_iscluster[i] = true;
			v_pnt.push_back(pnt);
			vv_pnt.push_back(v_pnt);
		}
	}
	return true;
}

/** \brief 	��������ָ��ƽ��
   * \param[in] pointcloud  ���ָ�ĵ��� 
   * \param[out] v_planecoeff  �洢�ָ���ÿ��ƽ���ϵ��
   * \param[out] v_vec_point  �洢�ָ���ÿ��ƽ���������ĵ�
   * \param[out] is_savefile  �Ƿ񱣴�ָ��ĵ����ļ�
   */
bool segment_plane(const pcl::PointCloud<pcl::PointXYZ>::Ptr pointcloud, 
	VecPointIndices &v_pointindices, VecPlaneCoeff &v_planecoeff, std::vector<VecPoint> &v_vec_point, bool is_savefile)
{
	pcl::PointCloud<pcl::PointXYZ>::Ptr p_pointcloud(new pcl::PointCloud<pcl::PointXYZ>());
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_f(new pcl::PointCloud<pcl::PointXYZ>());

	pcl::SACSegmentation<pcl::PointXYZ> seg;
	pcl::PointIndices::Ptr inliers (new pcl::PointIndices);
	pcl::ModelCoefficients::Ptr coefficients (new pcl::ModelCoefficients);
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_plane (new pcl::PointCloud<pcl::PointXYZ> ());
	seg.setOptimizeCoefficients (true);
	seg.setModelType (pcl::SACMODEL_PLANE);
	seg.setMethodType (pcl::SAC_RANSAC);
	seg.setMaxIterations (MAX_ITERS);
	seg.setDistanceThreshold (SEG_DIST_THRESHOLD);

	*p_pointcloud = *pointcloud;
	int i=0, nr_points = (int) p_pointcloud->points.size ();
	int index = 0;

	while (p_pointcloud->points.size () > 0.20 * nr_points)
	{
		// Segment the largest planar component from the remaining cloud
		seg.setInputCloud (p_pointcloud);
		seg.segment (*inliers, *coefficients);
		if (inliers->indices.size () == 0)
		{
			std::cout << "Could not estimate a planar model for the given dataset." << std::endl;
			return false;
		}

		//��ȡ�������ƽ���ڵ�
		pcl::ExtractIndices<pcl::PointXYZ> extract;
		extract.setInputCloud (p_pointcloud);
		extract.setIndices (inliers);
		extract.setNegative (false);

		// Write the planar inliers to disk
		extract.filter (*cloud_plane);
		if (is_savefile)
			std::cout << "PointCloud representing the planar component: " << cloud_plane->points.size () << " data points." << std::endl;

		//��ʼ�������ƽ�������
		{
			v_pointindices.push_back(*inliers);
			v_planecoeff.push_back(PlaneCoeff(coefficients->values[0], coefficients->values[1], coefficients->values[2], coefficients->values[3]));
			VecPoint v_point;
			for (size_t i = 0, count = cloud_plane->size(); i < count; ++i)
			{
				v_point.push_back(Point(cloud_plane->points[i].x, cloud_plane->points[i].y, cloud_plane->points[i].z));
			}		
			v_vec_point.push_back(v_point);

			if (is_savefile)
			{
				std::stringstream ss;
				ss << "cloud_seg_plane_" << index << ".pcd";
				index++;

				string str = g_str;
				str += "/";
				str += string(ss.str());

				io::savePCDFileASCII(str, *cloud_plane);
			}			
		}
		// ɾ��ƽ���ڵ㣬��ȡʣ��ĵ���
		extract.setNegative (true);
		extract.filter (*cloud_f);
		p_pointcloud.swap (cloud_f);
	}  // while (...)

	return true;
}

/** \brief 	��������ָ��ƽ��
   * \param[in] pointcloud  ���ָ�ĵ��� 
   * \param[out] v_planecoeff  �洢�ָ���ÿ��ƽ���ϵ��
   * \param[out] v_vec_point  �洢�ָ���ÿ��ƽ���������ĵ�
   * \param[out] is_savefile  �Ƿ񱣴�ָ��ĵ����ļ�
   */
bool segment_plane(const pcl::PointCloud<pcl::PointXYZ>::Ptr pointcloud, 
	VecPointIndices &v_pointindices, VecPlaneCoeff &v_planecoeff, std::vector<VecPoint> &v_vec_point, bool is_savefile, size_t pln_index)
{
	pcl::PointCloud<pcl::PointXYZ>::Ptr p_pointcloud(new pcl::PointCloud<pcl::PointXYZ>());
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_f(new pcl::PointCloud<pcl::PointXYZ>());

	pcl::SACSegmentation<pcl::PointXYZ> seg;
	pcl::PointIndices::Ptr inliers (new pcl::PointIndices);
	pcl::ModelCoefficients::Ptr coefficients (new pcl::ModelCoefficients);
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_plane (new pcl::PointCloud<pcl::PointXYZ> ());
	seg.setOptimizeCoefficients (true);
	seg.setModelType (pcl::SACMODEL_PLANE);
	seg.setMethodType (pcl::SAC_RANSAC);
	seg.setMaxIterations (100);
	seg.setDistanceThreshold (0.06);

	*p_pointcloud = *pointcloud;
	int i=0, nr_points = (int) p_pointcloud->points.size ();
	int index = 0;

	while (p_pointcloud->points.size () > 0.1 * nr_points)
	{
		// Segment the largest planar component from the remaining cloud
		seg.setInputCloud (p_pointcloud);
		seg.segment (*inliers, *coefficients);
		if (inliers->indices.size () == 0)
		{
			std::cout << "Could not estimate a planar model for the given dataset." << std::endl;
			return false;
		}

		//��ȡ�������ƽ���ڵ�
		pcl::ExtractIndices<pcl::PointXYZ> extract;
		extract.setInputCloud (p_pointcloud);
		extract.setIndices (inliers);
		extract.setNegative (false);

		// Write the planar inliers to disk
		extract.filter (*cloud_plane);
		if (is_savefile)
			std::cout << "PointCloud representing the planar component: " << cloud_plane->points.size () << " data points." << std::endl;

		//��ʼ�������ƽ�������
		{
			v_pointindices.push_back(*inliers);
			v_planecoeff.push_back(PlaneCoeff(coefficients->values[0], coefficients->values[1], coefficients->values[2], coefficients->values[3]));
			VecPoint v_point;
			for (size_t i = 0, count = cloud_plane->size(); i < count; ++i)
			{
				v_point.push_back(Point(cloud_plane->points[i].x, cloud_plane->points[i].y, cloud_plane->points[i].z));
			}		
			v_vec_point.push_back(v_point);

			if (is_savefile)
			{
				std::stringstream ss;
				ss << "cloud_seg_plane_"<<pln_index<<"_" << index << ".pcd";
				index++;

				string str = g_str;
				str += "/";
				str += string(ss.str());

				io::savePCDFileASCII(str, *cloud_plane);
			}			
		}
		// ɾ��ƽ���ڵ㣬��ȡʣ��ĵ���
		extract.setNegative (true);
		extract.filter (*cloud_f);
		p_pointcloud.swap (cloud_f);
	}  // while (...)

	return true;
}


 /** \brief  �ж�����ƽ���Ƿ����ƽ��
   * \param[in]  plane1  ��һ��ƽ��
   * \param[in] plane2  �ڶ���ƽ��
   */
bool is_parallel_plane(const PointCloudPlane &plane1, const PointCloudPlane &plane2)
{
	/*
	 *	ע����������ƽ��ķ��������Ѿ��ǵ�λ������
	 *  �������������λ�������Ƿ������ȵķ���Ч����̫�ã����ü�������������ļнǣ����н�С��ĳ����ֵ�������Ϊƽ��ƽ��
	 */
#ifdef _METHOD_EQUAL_
	float a_diff = plane1.m_coeff.a - plane2.m_coeff.a;
	float b_diff = plane1.m_coeff.b - plane2.m_coeff.b;
	float c_diff = plane1.m_coeff.c - plane2.m_coeff.c;
	float squre_sum = sqrt(a_diff * a_diff + b_diff * b_diff + c_diff * c_diff);

	if (-EPSILON <= squre_sum && squre_sum <= EPSILON){
		return true;
	}else{
		return false;
	}
#else
	float dot_product = plane1.m_coeff.a * plane2.m_coeff.a + plane1.m_coeff.b * plane2.m_coeff.b + plane1.m_coeff.c * plane2.m_coeff.c;
	dot_product = fabsf(dot_product);
	return dot_product >= cosf(ANGLE_THRESHOLD);
#endif
}

//Gaussian �˺���
static float Gauss(float x, float sigma)
{
	return exp (- (x*x)/(2*sigma*sigma));
}

 /** \brief  ����ƽ��ƽ���ľ���
   * \param[in] plane1  ��һ��ƽ��
   * \param[in] plane2  �ڶ���ƽ��
   * \param[in] mean_dist  ƽ���ļ�Ȩƽ������
   * \param[in] centers_dist  ��ƽ�����ĵľ���
   */
bool calculate_dist_parallel_planes(const PointCloudPlane &plane1, const PointCloudPlane &plane2, float &mean_dist, float &centers_dist)
{
	if ( !is_parallel_plane(plane1, plane2))
	{
		std::cerr<<"ָ��������ƽ�治����ƽ��"<<std::endl;
		return false;
	}
#ifdef _METHOD_FORMULA_
	//!ֱ�Ӱ���ѧ��ʽ���㣬�������׼ȷ
	mean_dist = fabsf(plane1.m_coeff.d - plane2.m_coeff.d);
#else
	//!������ø�˹��Ȩ����ƽ���ľ���
	VecPoint *vp_pnt = NULL, *vp_pnt2 = NULL;
	PlaneCoeff planecoeff;

	if (plane1.mv_pointcloud.size() <= plane2.mv_pointcloud.size())
	{
		vp_pnt = (VecPoint *)&(plane1.mv_pointcloud);
		planecoeff = plane2.m_coeff;

		vp_pnt2 = (VecPoint *)&(plane2.mv_pointcloud);
	} 
	else
	{
		vp_pnt = (VecPoint *)&plane2.mv_pointcloud;
		planecoeff = plane1.m_coeff;

		vp_pnt2 = (VecPoint *)&(plane1.mv_pointcloud);
	}

	float x1 = 0, y1 = 0, z1 = 0;
	float x2 = 0, y2 = 0, z2 = 0;
	pcl::PointCloud<pcl::PointXYZ>::Ptr in_cloud(new pcl::PointCloud<pcl::PointXYZ>());
	size_t count = vp_pnt->size();
	for (size_t i = 0; i < count; ++i)
	{
		pcl::PointXYZ basic_point;  

		basic_point.x = (*vp_pnt)[i].x;  
		basic_point.y = (*vp_pnt)[i].y;  
		basic_point.z = (*vp_pnt)[i].z;  
		in_cloud->points.push_back(basic_point);  

		x1 += (*vp_pnt)[i].x;
		y1 += (*vp_pnt)[i].y;
		z1 += (*vp_pnt)[i].z;
	}
	in_cloud->width = (int)in_cloud->points.size();  
	in_cloud->height = 1;  

	x1 /= count;
	y1 /= count;
	z1 /= count;

	size_t count2 = vp_pnt2->size();
	for (size_t i = 0; i < count2; ++i)
	{
		x2 += (*vp_pnt2)[i].x;
		y2 += (*vp_pnt2)[i].y;
		z2 += (*vp_pnt2)[i].z;
	}
	x2 /= count2;
	y2 /= count2;
	z2 /= count2;

	// Set up KDTree
	pcl::KdTreeFLANN<PointXYZ>::Ptr tree (new pcl::KdTreeFLANN<PointXYZ>);
	tree->setInputCloud (in_cloud);

	// Neighbors containers
	std::vector<int> k_indices;
	std::vector<float> k_distances;

	// Main Loop
	float radius = 4.0;
	float sum_weigh = 0;
	float sum_weigh_val = 0;
	float A = planecoeff.a, B = planecoeff.b, C = planecoeff.c, D = planecoeff.d;
	for (int point_id = 0, pnumber = in_cloud->size(); point_id < pnumber; ++point_id)
	{
		tree->radiusSearch (point_id, radius, k_indices, k_distances);

		float nbhd_count = k_distances.size();
		sum_weigh_val += fabsf(A * in_cloud->points[point_id].x + B * in_cloud->points[point_id].y + C *in_cloud->points[point_id].z + D) * nbhd_count;
		sum_weigh += nbhd_count;
	}
	mean_dist = sum_weigh_val / sum_weigh;

	float diff_x = x1 - x2, diff_y = y1 - y2, diff_z = z1 - z2;
	centers_dist = sqrt(diff_x * diff_x + diff_y * diff_y + diff_z * diff_z);
#endif

	return true;
}

 /** \brief  ��ƽ���Ҽ���̵�ƽ����ൽһ��
   * \param[in]  v_pntcldpln_before ����֮ǰ��ƽ��
   * \param[out]  v_vec_pntcldpln_after ����֮���ƽ����
   */
bool cluster_parallel_plane(const VecPointCloudPlane &v_pntcldpln_before, std::vector<VecPointCloudPlane> &v_vec_pntcldpln_after)
{
	const size_t plane_count = v_pntcldpln_before.size();
	std::vector<bool> v_iscluster(plane_count, false);

	for (size_t i = 0; i < plane_count; ++i)
	{
		for (size_t j = i + 1; j < plane_count; ++j)
		{
			PointCloudPlane pntcloudplane1 = v_pntcldpln_before[i];
			PointCloudPlane pntcloudplane2 = v_pntcldpln_before[j];
			float dist = FLT_MAX, centers_dist = FLT_MAX;
			
			if (is_parallel_plane(pntcloudplane1, pntcloudplane2))
			{
				if ( !calculate_dist_parallel_planes(pntcloudplane1, pntcloudplane2, dist, centers_dist))
				{
					return false;
				}
				//��ƽ��ɽ����غ�
				if ( dist < PLN2PLN_THRESHOLD && centers_dist < CENTER2CENTER_THRESHOLD)
				{
					VecPointCloudPlane v_pcp;

					if ( !v_iscluster[i] &&  !v_iscluster[j])
					{
						v_iscluster[i] = true;
						v_pcp.push_back(pntcloudplane1);

						v_iscluster[j] = true;
						v_pcp.push_back(pntcloudplane2);

						v_vec_pntcldpln_after.push_back(v_pcp);
					}
					else if ( v_iscluster[i] &&  !v_iscluster[j])
					{
						size_t clusterd_count = v_vec_pntcldpln_after.size();
						size_t m, n;
						for (m = 0; m < clusterd_count; ++m)
						{
							size_t count = v_vec_pntcldpln_after[m].size();
							for (n = 0; n < count; ++n)
							{
								if (pntcloudplane1.m_indice == v_vec_pntcldpln_after[m][n].m_indice)
								{
									v_vec_pntcldpln_after[m].push_back(pntcloudplane2);
									v_iscluster[j] = true;
									break;
								}
							}  // for n
						}  // for m
					}  // else if
					else if ( !v_iscluster[i] &&  v_iscluster[j])
					{
						size_t clusterd_count = v_vec_pntcldpln_after.size();
						size_t m, n;
						for (m = 0; m < clusterd_count; ++m)
						{
							size_t count = v_vec_pntcldpln_after[m].size();
							for (n = 0; n < count; ++n)
							{
								if (pntcloudplane2.m_indice == v_vec_pntcldpln_after[m][n].m_indice)
								{
									v_vec_pntcldpln_after[m].push_back(pntcloudplane1);
									v_iscluster[i] = true;
									break;
								}
							}  // for n
						}  // for m
					}
					else
					{
						//���Ѿ����࣬�Ͳ������������Ի����A��B����ƽ�У�B��C����ƽ�У���A��C���ܲ�����ƽ�У�Ŀǰ�����������������
					}
				}  //  dist < PLN2PLN_THRESHOLD
			}  //  if (is_parallel_plane(pntcloudplane1, pntcloudplane2))

		}  //  for j
	}  //  for i

	for (size_t i = 0; i < plane_count; ++i)
	{
		PointCloudPlane pntcloudplane = v_pntcldpln_before[i];

		if (!v_iscluster[i])
		{
			VecPointCloudPlane v_pcp;
			v_iscluster[i] = true;
			v_pcp.push_back(pntcloudplane);
			v_vec_pntcldpln_after.push_back(v_pcp);
		}
	}

	return true;
}

 /** \brief  ����һ��Vector������洢PointCloudPlane
   * \param[in]  v_vec_point  ƽ���������ĵ���
   * \param[in]  v_planecoeff  ƽ�淽��ϵ��
   * \param[out]  v_pcp  �洢PointCloudPlane������
   */
bool build_vec_pointcloudplane(const std::vector<VecPoint> &v_vec_point, const VecPlaneCoeff &v_planecoeff, VecPointCloudPlane &v_pcp)
{
	size_t count = v_planecoeff.size();

    assert(v_vec_point.size() == count);

	for (size_t i = 0; i < count; ++i)
	{
		PointCloudPlane pcp(i, v_vec_point[i], v_planecoeff[i]);
		v_pcp.push_back(pcp);
	}

	return true;
}

 /** \brief  ������ƽ�е�ƽ��鲢��һ��ƽ�棻�����ƽ���ϵĵ�ͶӰ������һ��ƽ����
   * \param[in]  v_pcp  �洢����ƽ�е�ƽ��
   * \param[out]  pntcloudplane  �鲢���ƽ��
   */
bool merge_mult_parallel_planes(VecPointCloudPlane &v_pcp, PointCloudPlane &pntcloudplane)
{
    size_t count = v_pcp.size(), index = 0;

#ifdef _BASE_MAX_NUMS
	//ѡȡ������ƽ������׼
	size_t max_pnt_count = v_pcp[0].mv_pointcloud.size();
	for (size_t i = 0; i < count; ++i)
	{
		size_t pnt_cnt = v_pcp[i].mv_pointcloud.size();
		if (max_pnt_count < pnt_cnt)
		{
			max_pnt_count = pnt_cnt;
			index = i;
		}
	}
#elif defined _BASE_VERTICLE_GROUND
	//����ӽ���ֱ�ڵ����ƽ��ѡ����׼, ƽ�淨������(0, 0, 1)�ĵ��Խ�ӽ���0���ͱ�ʾ��ƽ��Խ�ӽ���ֱ�ڵ���
	float min_dot = fabsf(v_pcp[0].m_coeff.c);
	for (size_t i = 0; i < count; ++i)
	{
		float dotproj = fabsf(v_pcp[i].m_coeff.c);
		if (min_dot > dotproj)
		{
			min_dot = dotproj;
			index = i;
		}
	}
#else
#error ���붨��_BASE_MAX_NUMS��_BASE_VERTICLE_GROUND�е�һ��
#endif

	//������ƽ��ĵ�ͶӰ���±�Ϊindex��ƽ����
	float A = v_pcp[index].m_coeff.a;
	float B = v_pcp[index].m_coeff.b;
	float C = v_pcp[index].m_coeff.c;
	float D = v_pcp[index].m_coeff.d;
	for (size_t i = 0; i < count; ++i)
	{
		VecPoint & v_pnt = v_pcp[i].mv_pointcloud;
		size_t pnt_count = v_pnt.size();

		for (size_t j = 0; j < pnt_count; ++j)
		{
			float negtive_k = A * v_pnt[j].x +B * v_pnt[j].y + C * v_pnt[j].z + D;
			float x = v_pnt[j].x - A * negtive_k;
			float y = v_pnt[j].y - B * negtive_k;
			float z = v_pnt[j].z - C * negtive_k;

			pntcloudplane.mv_pointcloud.push_back(Point(x, y, z));
		}
	}
	pntcloudplane.m_indice = v_pcp[index].m_indice;
	pntcloudplane.m_coeff = v_pcp[index].m_coeff;

	return true;
}

 /** \brief  ��PointCloudPlaneת��ΪPointCloud
   * \param[in]  in_pcp �����PointCloudPlane
   * \param[out]  cld �����PointCloud
   */
bool pointcloudplane2pointcloud(const PointCloudPlane & in_pcp, PointCloud<PointXYZ>::Ptr cld)
{
	size_t pnt_count = in_pcp.mv_pointcloud.size();
	for (size_t j = 0; j < pnt_count; ++j)
	{
		pcl::PointXYZ basic_point;  

		basic_point.x = in_pcp.mv_pointcloud[j].x;
		basic_point.y = in_pcp.mv_pointcloud[j].y;  
		basic_point.z = in_pcp.mv_pointcloud[j].z;
		cld->points.push_back(basic_point);  
	}
	cld->width = (int)cld->points.size();  
	cld->height = 1;
	return true;
}

 /** \brief  ��PointCloudת��ΪPointCloudPlane
   * \param[in]  in_cld �����PointCloud
   * \param[in]  indice �����ƽ����±�����
   * \param[in]  planecoeff �����ƽ��ϵ��
   * \param[out]  pcp �����PointCloudPlane
   */
bool pointcloud2pointcloudplane(const PointCloud<PointXYZ>::Ptr in_cld,  const size_t &indice, const PlaneCoeff &planecoeff, PointCloudPlane & pcp)
{
	size_t pnt_count = in_cld->points.size();
	for (size_t j = 0; j < pnt_count; ++j)
	{
		Point basic_point;  

		basic_point.x = in_cld->points[j].x;
		basic_point.y = in_cld->points[j].y;  
		basic_point.z = in_cld->points[j].z;
		pcp.mv_pointcloud.push_back(basic_point);  
	}
	pcp.m_coeff.a = planecoeff.a;
	pcp.m_coeff.b = planecoeff.b;
	pcp.m_coeff.c = planecoeff.c;
	pcp.m_coeff.d = planecoeff.d;

	pcp.m_indice = indice;

	return true;
}

 /** \brief  �Ƴ�����ƽ���е����(��Ⱥ��)
   * \param[in]  in_pcp ����ĵ���ƽ��
   * \param[in]  R �뾶
   * \param[in]  min_nbhd_num ��R�뾶�����ڰ�Χ�ĵ�����С����
   * \param[out]  out_pcp ���������ĵ���ƽ��
   */
bool rmv_outliers(const PointCloudPlane & in_pcp, const float R, const size_t min_nbhd_num, PointCloudPlane & out_pcp)
{
	pcl::RadiusOutlierRemoval<pcl::PointXYZ> outrem;
	PointCloud<PointXYZ>::Ptr cld(new PointCloud<PointXYZ>());
	PointCloud<PointXYZ>::Ptr cloud_filtered(new PointCloud<PointXYZ>());

	pointcloudplane2pointcloud(in_pcp, cld);

	// build the filter
	outrem.setInputCloud(cld);
	outrem.setRadiusSearch(R);
	outrem.setMinNeighborsInRadius (min_nbhd_num);
	// apply filter
	outrem.filter (*cloud_filtered);

	pointcloud2pointcloudplane(cloud_filtered, in_pcp.m_indice, in_pcp.m_coeff, out_pcp);
	return true;
}

 /** \brief  �����洢��ƽ�е�ƽ���壬����ƽ��ƽ�����е�ƽ��鲢
   * \param[in]  v_vec_pntcldpln �洢��ƽ��ƽ����
   * \param[out]  v_pntcldpln �鲢֮��ķ�ƽ��ƽ����
   */
bool traverse_merge_planes(std::vector<VecPointCloudPlane> & v_vec_pntcldpln, VecPointCloudPlane & v_pntcldpln)
{
    size_t vec_pcp_count = v_vec_pntcldpln.size();
	float R = 2.5;
	size_t min_nbhd_num = 25;

	for (size_t i = 0; i < vec_pcp_count; ++i)
	{
		PointCloudPlane pntcldpln, pntcldpln_rmvoutliers;
		merge_mult_parallel_planes(v_vec_pntcldpln[i], pntcldpln);
		rmv_outliers(pntcldpln, R, min_nbhd_num, pntcldpln_rmvoutliers);

		v_pntcldpln.push_back(pntcldpln_rmvoutliers);
	}

	return true;
}

 /** \brief  ����������ͬ�ĵ��ȡһ������������������ƽ����XOYƽ��
   * \param[in]  pnt0
   * \param[in]  pnt1
   * \param[in]  pnt2
   * \param[out]  vec �õ��ķ�������
   */
inline bool get_parallel_ground_vector(const Point &pnt0, const Point &pnt1, const Point &pnt2, Vector &vec)
{
	if (pnt0 == pnt1 || pnt0 == pnt2 || pnt1 == pnt2)
	{
		std::cerr<<"���������������ͬ!"<<std::endl;
		return false;
	}

	float dx1 = pnt1.x - pnt0.x;
	float dy1 = pnt1.y - pnt0.y;
	float dz1 = pnt1.z - pnt0.z;

	float dx2 = pnt1.x - pnt2.x;
	float dy2 = pnt1.y - pnt2.y;
	float dz2 = pnt1.z - pnt2.z;

	//ƽ���ڵ���ķ���������(m, n, 0)
	float m = dx1 - dz1/ dz2 * dx2;
	float n = dy1 - dz1/ dz2 * dy2;

	vec.x = m;
	vec.y = n;
	vec.z = 0;
	vec.normalize();

	return true;
}

 /** \brief  ����ƽ��ķ�������ȡһ������������������ƽ����XOYƽ��
   * \param[in]  plncoeff ƽ�淽��ϵ��
   * \param[out]  vec �õ��ķ�������
   */
inline bool get_parallel_ground_vector(const PlaneCoeff & plncoeff, Vector &vec)
{
	//ƽ���ڵ���ķ���������(m, n, 0)
	float m = plncoeff.b;
	float n = -plncoeff.a;

	vec.x = m;
	vec.y = n;
	vec.z = 0;
	vec.normalize();

	return true;
}

 /** \brief  ����������ͬ���ȡƽ�淽��ϵ��
   * \param[] 
   * \param[] 
   */
static bool get_planecoeff(const Point &pnt1, const Point &pnt2, const Point &pnt3, PlaneCoeff &plncoeff)
{
	Eigen::Vector3f v1(pnt2.x - pnt1.x, pnt2.y - pnt1.y, pnt2.z - pnt1.z);
	Eigen::Vector3f v2(pnt2.x - pnt3.x, pnt2.y - pnt3.y, pnt2.z - pnt3.z);
	Eigen::Vector3f normal = v1.cross(v2);
	normal.normalize();
	float d = -(normal[0] * pnt1.x + normal[1] * pnt1.y + normal[2] * pnt1.z);

	plncoeff.a = normal[0];
	plncoeff.b = normal[1];
	plncoeff.c = normal[2];
	plncoeff.d = d;

	return true;
}

 /** \brief  ȷ��pnt0��pnt2����pnt1�ķ��������ϵ�ͶӰ�㣬���з�������ƽ����XOY��ƽ��
   * \param[in]  pnt0
   * \param[in]  pnt1
   * \param[in]  pnt2
   * \param[out]  proj_pnt0
   * \param[out]  proj_pnt2
   */
static bool determin_proj(const Point &pnt0, const Point &pnt1, const Point &pnt2, Point &proj_pnt0, Point &proj_pnt2)
{
	Vector direction_vector;
	get_parallel_ground_vector(pnt0, pnt1, pnt2, direction_vector);

	//ƽ���ڵ���ķ���������(m, n, 0)
	float m = direction_vector.x;
	float n = direction_vector.y;

	float k_numerator = 0;
	float k = 0;
	float xp = 0, yp = 0;

	k_numerator = m * (pnt0.x - pnt1.x)	+ n * (pnt0.y - pnt1.y);	
	k = k_numerator / (m * m + n * n);
	xp = k * m +pnt1.x;
	yp = k * n + pnt1.y;
	proj_pnt0.x = xp;
	proj_pnt0.y = yp;
	proj_pnt0.z = pnt1.z;

	k_numerator = m * (pnt2.x - pnt1.x)	+ n * (pnt2.y - pnt1.y);	
	k = k_numerator / (m * m + n * n);
	xp = k * m +pnt1.x;
	yp = k * n + pnt1.y;
	proj_pnt2.x = xp;
	proj_pnt2.y = yp;
	proj_pnt2.z = pnt1.z;

	return true;
}

 /** \brief  ȷ��pnt0��pnt2����pnt1�ķ��������ϵ�ͶӰ�㣬���з�������ƽ����XOY��ƽ��
   * \param[in]  pnt0
   * \param[in]  pnt1
   * \param[in]  pnt2
   * \param[out]  proj_pnt0
   * \param[out]  proj_pnt2
   */
static bool determin_proj(const Point &pnt0, const Point &pnt1, const Point &pnt2, const Vector & direction_vector, Point &proj_pnt0, Point &proj_pnt2)
{
	//ƽ���ڵ���ķ���������(m, n, 0)
	float m = direction_vector.x;
	float n = direction_vector.y;

	float k_numerator = 0;
	float k = 0;
	float xp = 0, yp = 0;

	k_numerator = m * (pnt0.x - pnt1.x)	+ n * (pnt0.y - pnt1.y);	
	k = k_numerator / (m * m + n * n);
	xp = k * m +pnt1.x;
	yp = k * n + pnt1.y;
	proj_pnt0.x = xp;
	proj_pnt0.y = yp;
	proj_pnt0.z = pnt1.z;

	k_numerator = m * (pnt2.x - pnt1.x)	+ n * (pnt2.y - pnt1.y);	
	k = k_numerator / (m * m + n * n);
	xp = k * m +pnt1.x;
	yp = k * n + pnt1.y;
	proj_pnt2.x = xp;
	proj_pnt2.y = yp;
	proj_pnt2.z = pnt1.z;

	return true;
}

 /** \brief  ȷ��ƽ���3������(ʹ�������ѡȡ��ķ�ʽ)
 * \param[in]  pntcldpln �����ƽ��
   * \param[out]  rect �������ƽ���3������
   */
bool determin_plane_verticles_userand(const PointCloudPlane & pntcldpln, Rect &rect)
{
	VecPoint &v_point = (VecPoint &)pntcldpln.mv_pointcloud;
	PlaneCoeff &planecoeff = (PlaneCoeff &)pntcldpln.m_coeff;
	size_t highest_point_index = 0, bound1_index = 0, bound2_index = 0;
	size_t lowest_point_index = 0;
	size_t pnt_count = v_point.size();

	//��ý�����ƽ�����ߵ㣬�Լ�������ƽ�����͵�
	float max_z = v_point[0].z, min_z = v_point[0].z;
	for (size_t i = 0; i < pnt_count; ++i)
	{
		if (max_z < v_point[i].z)
		{
			max_z = v_point[i].z;
			highest_point_index = i;
		}
		if (min_z > v_point[i].z)
		{
			min_z = v_point[i].z;
			lowest_point_index = i;
		}
	}
	bound1_index = highest_point_index;
	bound2_index = highest_point_index;

	//����ƽ����������ȷ��ƽ����ε��ϱߵķ����������÷�������ƽ���ڵ���
	//֮ǰ��˼·�ǣ�ƽ��ķ�������(0, 0, 1)�Ĳ����Ϊ������ƽ����߱����ڷ������������ַ���������ȷ
	size_t rand_index1 = 0, rand_index2 = 0;
	do 
	{
#ifdef _RAND
		srand((unsigned)time(NULL));
#endif
		rand_index1 = static_cast<size_t>(rand() % pnt_count);
		rand_index2 = static_cast<size_t>(rand() % pnt_count);
	} while (rand_index1 == highest_point_index || rand_index2 == highest_point_index);

	Vector direction_vector;  //!������ƽ����ߵı����ڷ�������

	get_parallel_ground_vector(v_point[rand_index1], v_point[highest_point_index], v_point[rand_index2], direction_vector);

	float max_dist1 = 0, max_dist2 = 0;
	
	//��ȡ������ƽ���������2������
	for (size_t i = 0; i < pnt_count; ++i)
	{
		Vector between_points_vector;
		between_points_vector.x = v_point[i].x - v_point[highest_point_index].x;
		between_points_vector.y = v_point[i].y - v_point[highest_point_index].y;
		between_points_vector.z = v_point[i].z - v_point[highest_point_index].z;
		
		float dot_product = between_points_vector.x * direction_vector.x + between_points_vector.y * direction_vector.y
			+ between_points_vector.z * direction_vector.z;

		float dist = 0;
		if (dot_product > 0)
		{
			dist = dot_product;
			if (max_dist1 < dist)
			{
				max_dist1 = dist;
				bound1_index = i;
			}
		} 
		else if (dot_product < 0)
		{
			dist = -dot_product;
			if (max_dist2 < dist)
			{
				max_dist2 = dist;
				bound2_index = i;
			}
		}
	}  // for (size_t i = 0; i < pnt_count; ++i)

	/*
	 *	��highest_point_index, bound1_index, bound2_index, lowest_point_indexȷ��һ�����α߿�
	 *	���������ε�������ı�ƽ���ڵ��棬����������(m, n, 0)
	 */
	Point proj_bound1, proj_bound2;
	Vector v_verticle;
	if (bound1_index != highest_point_index && bound2_index != highest_point_index)
	{
		determin_proj(v_point[bound1_index], v_point[highest_point_index], v_point[bound2_index], proj_bound1, proj_bound2);
		v_verticle.x = v_point[bound1_index].x - proj_bound1.x;
		v_verticle.y = v_point[bound1_index].y - proj_bound1.y;
		v_verticle.z = v_point[bound1_index].z - proj_bound1.z;
	} 
	else if (bound1_index == highest_point_index)
	{
		size_t rand_index = 0;
		do 
		{
#ifdef _RAND
			srand((unsigned)time(NULL));
#endif
			rand_index = static_cast<size_t>(rand() % pnt_count);
		} while (rand_index == bound1_index);

		determin_proj(v_point[rand_index], v_point[highest_point_index], v_point[bound2_index], proj_bound1, proj_bound2);
		proj_bound1 = v_point[highest_point_index];

		v_verticle.x = v_point[bound2_index].x - proj_bound2.x;
		v_verticle.y = v_point[bound2_index].y - proj_bound2.y;
		v_verticle.z = v_point[bound2_index].z - proj_bound2.z;
	}
	else if (bound2_index == highest_point_index)
	{
		size_t rand_index = 0;
		do 
		{
#ifdef _RAND
			srand((unsigned)time(NULL));
#endif
			rand_index = static_cast<size_t>(rand() % pnt_count);
		} while (rand_index == bound2_index);
		determin_proj(v_point[bound1_index], v_point[highest_point_index], v_point[rand_index], proj_bound1, proj_bound2);
		proj_bound2 = v_point[highest_point_index];

		v_verticle.x = v_point[bound1_index].x - proj_bound1.x;
		v_verticle.y = v_point[bound1_index].y - proj_bound1.y;
		v_verticle.z = v_point[bound1_index].z - proj_bound1.z;
	}
	
	//���濪ʼ��verticle3������
	v_verticle.normalize();

	float k = 0;
	float verticle3_x = 0, verticle3_y = 0, verticle4_x = 0, verticle4_y = 0;
	k = (v_point[lowest_point_index].z - proj_bound2.z) / v_verticle.getVecLength();
	verticle3_x = k * v_verticle.x + proj_bound2.x;
	verticle3_y = k * v_verticle.y + proj_bound2.y;

	k = (v_point[lowest_point_index].z - proj_bound1.z) / v_verticle.getVecLength();
	verticle4_x = k * v_verticle.x + proj_bound1.x;
	verticle4_y = k * v_verticle.y + proj_bound1.y;

	rect.m_indice = pntcldpln.m_indice;
	rect.m_verticle1 = proj_bound1;
	rect.m_verticle2 = proj_bound2;
	rect.m_verticle3 = Point(verticle3_x, verticle3_y, v_point[lowest_point_index].z);
	rect.m_verticle4 = Point(verticle4_x, verticle4_y, v_point[lowest_point_index].z);

	//PlaneCoeff plncoeff, coeff;
	//get_planecoeff(rect.m_verticle1, rect.m_verticle2, rect.m_verticle3, plncoeff);
	//get_planecoeff(v_point[bound1_index], v_point[highest_point_index], v_point[bound2_index], coeff);

	return true;
}

 /** \brief ��ȡƽ���һЩ"������"���ֱ�����ߵ㡢��͵㡢����߽��
   * \param[in] pntcldpln ƽ��
   * \param[out] highest_index ��ߵ���±�����
   * \param[out] lowest_index ��͵���±�����
   * \param[out] bound1_index ����߽����±�����
   * \param[out] bound2_index ����߽����±�����
   */
static bool get_plane_feature_points(const PointCloudPlane &pntcldpln, size_t &highest_index, size_t &lowest_index, size_t &bound1_index, size_t &bound2_index)
{
	VecPoint &v_point = (VecPoint &)pntcldpln.mv_pointcloud;
	PlaneCoeff &planecoeff = (PlaneCoeff &)pntcldpln.m_coeff;
	size_t pnt_count = v_point.size();

	//��ý�����ƽ�����ߵ㣬�Լ�������ƽ�����͵�
	float max_z = v_point[0].z, min_z = v_point[0].z;
	for (size_t i = 0; i < pnt_count; ++i)
	{
		if (max_z < v_point[i].z)
		{
			max_z = v_point[i].z;
			highest_index = i;
		}
		if (min_z > v_point[i].z)
		{
			min_z = v_point[i].z;
			lowest_index = i;
		}
	}
	bound1_index = highest_index;
	bound2_index = highest_index;

	//����ƽ����ߵ�(Z = Max)�Լ�ƽ��ķ�����ȷ��ƽ����ε��ϱߵķ����������÷�������ƽ���ڵ���
	//֮ǰ��˼·�ǣ�ƽ��ķ�������(0, 0, 1)�Ĳ����Ϊ������ƽ����߱����ڷ������������ַ���������ȷ
	Vector direction_vector;  //!������ƽ����ߵı����ڷ�������
	get_parallel_ground_vector(planecoeff, direction_vector);

	float max_dist1 = 0, max_dist2 = 0;

	//��ȡ������ƽ���������2������
	for (size_t i = 0; i < pnt_count; ++i)
	{
		Vector between_points_vector;
		between_points_vector.x = v_point[i].x - v_point[highest_index].x;
		between_points_vector.y = v_point[i].y - v_point[highest_index].y;
		between_points_vector.z = v_point[i].z - v_point[highest_index].z;

		float dot_product = between_points_vector.x * direction_vector.x + between_points_vector.y * direction_vector.y
			+ between_points_vector.z * direction_vector.z;

		float dist = 0;
		if (dot_product > 0)
		{
			dist = dot_product;
			if (max_dist1 < dist)
			{
				max_dist1 = dist;
				bound1_index = i;
			}
		} 
		else if (dot_product < 0)
		{
			dist = -dot_product;
			if (max_dist2 < dist)
			{
				max_dist2 = dist;
				bound2_index = i;
			}
		}
	}  // for (size_t i = 0; i < pnt_count; ++i)

	return true;
}

 /** \brief  ȷ��ƽ���3������
 * \param[in]  pntcldpln �����ƽ��
   * \param[out]  rect �������ƽ���3������
   */
bool determin_plane_verticles(/*const*/ PointCloudPlane & pntcldpln, Rect &rect)
{
	VecPoint &v_point = (VecPoint &)pntcldpln.mv_pointcloud;
	PlaneCoeff &planecoeff = (PlaneCoeff &)pntcldpln.m_coeff;
	size_t highest_point_index = 0, bound1_index = 0, bound2_index = 0;
	size_t lowest_point_index = 0;
	size_t pnt_count = v_point.size();
	
	Vector direction_vector;  //!������ƽ����ߵı����ڷ�������
	get_parallel_ground_vector(planecoeff, direction_vector);

	get_plane_feature_points(pntcldpln, highest_point_index, lowest_point_index, bound1_index, bound2_index);

	/*
	 *	��highest_point_index, bound1_index, bound2_index, lowest_point_indexȷ��һ�����α߿�
	 *	���������ε�������ı�ƽ���ڵ��棬����������(m, n, 0)
	 */
	Point proj_bound1, proj_bound2;
	Vector v_verticle;
	if (bound1_index != highest_point_index && bound2_index != highest_point_index)
	{
		determin_proj(v_point[bound1_index], v_point[highest_point_index], v_point[bound2_index], direction_vector, proj_bound1, proj_bound2);
		v_verticle.x = v_point[bound1_index].x - proj_bound1.x;
		v_verticle.y = v_point[bound1_index].y - proj_bound1.y;
		v_verticle.z = v_point[bound1_index].z - proj_bound1.z;
	} 
	else if (bound1_index == highest_point_index)
	{
		determin_proj(v_point[highest_point_index], v_point[highest_point_index], v_point[bound2_index], direction_vector, proj_bound1, proj_bound2);
		proj_bound1 = v_point[highest_point_index];

		v_verticle.x = v_point[bound2_index].x - proj_bound2.x;
		v_verticle.y = v_point[bound2_index].y - proj_bound2.y;
		v_verticle.z = v_point[bound2_index].z - proj_bound2.z;
	}
	else if (bound2_index == highest_point_index)
	{
		determin_proj(v_point[bound1_index], v_point[highest_point_index], v_point[highest_point_index], direction_vector, proj_bound1, proj_bound2);
		proj_bound2 = v_point[highest_point_index];

		v_verticle.x = v_point[bound1_index].x - proj_bound1.x;
		v_verticle.y = v_point[bound1_index].y - proj_bound1.y;
		v_verticle.z = v_point[bound1_index].z - proj_bound1.z;
	}
	
	//���濪ʼ��verticle3������
	v_verticle.normalize();

	float k = 0;
	float verticle3_x = 0, verticle3_y = 0, verticle4_x = 0, verticle4_y = 0;
	k = (v_point[lowest_point_index].z - proj_bound2.z) / v_verticle.getVecLength();
	verticle3_x = k * v_verticle.x + proj_bound2.x;
	verticle3_y = k * v_verticle.y + proj_bound2.y;

	k = (v_point[lowest_point_index].z - proj_bound1.z) / v_verticle.getVecLength();
	verticle4_x = k * v_verticle.x + proj_bound1.x;
	verticle4_y = k * v_verticle.y + proj_bound1.y;

	rect.m_indice = pntcldpln.m_indice;
	rect.m_verticle1 = proj_bound1;
	rect.m_verticle2 = proj_bound2;
	rect.m_verticle3 = Point(verticle3_x, verticle3_y, v_point[lowest_point_index].z);
	rect.m_verticle4 = Point(verticle4_x, verticle4_y, v_point[lowest_point_index].z);

	PlaneCoeff plncoeff, coeff;
	get_planecoeff(rect.m_verticle1, rect.m_verticle2, rect.m_verticle3, plncoeff);
	get_planecoeff(v_point[bound1_index], v_point[highest_point_index], v_point[bound2_index], coeff);

	//����ƽ��ϵ��
	pntcldpln.m_coeff = plncoeff;

	return true;
}

 /** \brief  ��ȡֱ����ƽ��Ľ���
   * \param[in] pnt1  ֱ�ߵ�һ�˵�
   * \param[in] pnt2  ֱ�ߵ���һ�˵�
   * \param[in] plncoeff  ƽ��
   * \param[out] intersected_pnt  ��ȡ�Ľ���
   */
bool get_intersected_pnt(const Point &pnt1, const Point &pnt2, const PlaneCoeff &plncoeff, Point &intersected_pnt)
{
	float pnt1_plane = pnt1.x * plncoeff.a + pnt1.y * plncoeff.b + pnt1.z * plncoeff.c + plncoeff.d;
	float k = pnt1_plane / (plncoeff.a * (pnt1.x - pnt2.x) + plncoeff.b * (pnt1.y - pnt2.y) + plncoeff.c * (pnt1.z - pnt2.z));
	std::cout<<"k = "<<k<<std::endl;
	//��������
	intersected_pnt.x = k * pnt2.x + (1 - k) * pnt1.x;
	intersected_pnt.y = k * pnt2.y + (1 - k) * pnt1.y;
	intersected_pnt.z = k * pnt2.z + (1 - k) * pnt1.z;

	return true;
}

//�ú����д��ع�
static bool get_plane_bound_frm_original_pointcloud(const PointCloud<PointXYZ>::Ptr pntcld_original, const PlaneCoeff &plncoeff, Point &bound1, Point &bound2)
{
	float a = plncoeff.a, b = plncoeff.b, c = plncoeff.c, d = plncoeff.d;
	Vector direction_vector;  //!������ƽ����ߵı����ڷ�������
	float max_z = -FLT_MAX, min_z = FLT_MAX;
	size_t highest_index = 0, lowest_index = 0, bound1_index = 0, bound2_index = 0;

	get_parallel_ground_vector(plncoeff, direction_vector);

	std::vector<PointXYZ, Eigen::aligned_allocator<PointXYZ> > &points = pntcld_original->points;
	size_t pntcount = points.size();

	//�ҵ�Zֵ��ߵ�����С��
	for (size_t i = 0; i < pntcount; ++i)
	{			
		if (fabsf(a * points[i].x + b * points[i].y + c * points[i].z + d) <= PNT2PLANE_THRESHOLD)
		{
			if (max_z < points[i].z)
			{
				max_z = points[i].z;
				highest_index = i;
			}
			if (min_z > points[i].z)
			{
				min_z = points[i].z;
				lowest_index = i;
			}
		}
	}  //  for (size_t i = 0; i < pntcount; ++i)

	bound1_index = highest_index;
	bound2_index = highest_index;

	//�ҵ�ƽ������ı߽��
	float max_dist1 = 0, max_dist2 = 0;
	for (size_t i = 0; i < pntcount; ++i)
	{
		if (fabsf(a * points[i].x + b * points[i].y + c * points[i].z + d) <= PNT2PLANE_THRESHOLD)
		{
			Vector between_points_vector;
			between_points_vector.x = points[i].x - points[highest_index].x;
			between_points_vector.y = points[i].y - points[highest_index].y;
			between_points_vector.z = points[i].z - points[highest_index].z;

			float dot_product = between_points_vector.x * direction_vector.x + between_points_vector.y * direction_vector.y
				+ between_points_vector.z * direction_vector.z;

			float dist = 0;
			if (dot_product > 0)
			{
				dist = dot_product;
				if (max_dist1 < dist)
				{
					max_dist1 = dist;
					bound1_index = i;
				}
			} 
			else if (dot_product < 0)
			{
				dist = -dot_product;
				if (max_dist2 < dist)
				{
					max_dist2 = dist;
					bound2_index = i;
				}
			}
		}  // for (size_t i = 0; i < pnt_count; ++i)
	}
	bound1.x = points[bound1_index].x;
	bound1.y = points[bound1_index].y;
	bound1.z = points[bound1_index].z;

	bound2.x = points[bound2_index].x;
	bound2.y = points[bound2_index].y;
	bound2.z = points[bound2_index].z;

	return true;
}

 /** \brief  ��ȡ�ü���ľ��α�
   * \param[in] pntcld_original ԭʼ����
   * \param[in] curr_plncoeff ��ǰƽ�淽��ϵ��
   * \param[in] plncoeff ƽ�淽��ϵ��
   * \param[in/out] pnt1 ��������һ������
   * \param[in/out] pnt2 ��������һ������
   */
static bool get_rect_edge_cut(const PointCloud<PointXYZ>::Ptr pntcld_original, const PlaneCoeff &curr_plncoeff, const PlaneCoeff &plncoeff, Point &pnt1, Point &pnt2)
{
	float dist = 0;
	int count = 0;

	float pnt1_plane = pnt1.x * plncoeff.a + pnt1.y * plncoeff.b + pnt1.z * plncoeff.c + plncoeff.d;
	if (pnt1_plane > NEAREST_ZERO){
		count++;
	}
	else if (pnt1_plane < -NEAREST_ZERO){
		count--;
	}
	else{
		//�Ѿ���ƽ������
		return true;
	}

	float pnt2_plane = pnt2.x * plncoeff.a + pnt2.y * plncoeff.b + pnt2.z * plncoeff.c + plncoeff.d;
	if (pnt2_plane > NEAREST_ZERO){
		count++;
	}
	else if (pnt2_plane < -NEAREST_ZERO){
		count--;
	}
	else{
		//�Ѿ���ƽ������
		return true;
	}

	//����ƽ��v_rect[i]�����ƽ��v_pntcldpln[j]�ཻ
	if (0 == count)
	{
		//����һ��������ƽ��ܽ�����Ҫ�ü�
		if (fabsf(pnt1_plane) < PNT2PLANE_THRESHOLD || fabsf(pnt2_plane) < PNT2PLANE_THRESHOLD)
		{
			std::cout<<"�߶���ƽ���ཻ"<<std::endl;
			Point intersected_pnt;
			get_intersected_pnt(pnt1, pnt2, plncoeff, intersected_pnt);

			if (fabsf(pnt1_plane) < fabsf(pnt2_plane))
			{
				DEBUG_PRINT(pnt1_plane, pnt1, pnt2, intersected_pnt);
				pnt1 = intersected_pnt;
			} 
			else
			{
				DEBUG_PRINT(pnt2_plane, pnt2, pnt1, intersected_pnt);
				pnt2 = intersected_pnt;
			}
		}
		else
		{
			//��������ƽ�涼��Զ����ʱ�������������һ����������ȷʵ��Ӧԭʼ����ƽ�棬
			//��һ����������һ������Ϊ֮ǰ�ָ�ʱ����׼ȷ����ͶӰʱͶ����ǰƽ����
			Point intersected_pnt;
			Point bound1, bound2;
			Vector vec_horizon;

			get_parallel_ground_vector(curr_plncoeff, vec_horizon);

			get_intersected_pnt(pnt1, pnt2, plncoeff, intersected_pnt);
			get_plane_bound_frm_original_pointcloud(pntcld_original, curr_plncoeff, bound1, bound2);

			//Ѱ����ʵ�Ķ���
			bool select_pnt1 = false, select_pnt2 = false, select_intersect = false;
			do 
			{
				if (comm_oper::makeVector(pnt1, bound1).normalize().isVertical(vec_horizon))
				{
					select_pnt1 = true;
					break;
				}
				if (comm_oper::makeVector(pnt2, bound1).normalize().isVertical(vec_horizon))
				{
					select_pnt2 = true;
					break;
				}
				if (comm_oper::makeVector(intersected_pnt, bound1).normalize().isVertical(vec_horizon))
				{
					select_intersect = true;
					break;
				}
			} while (0);

			do 
			{
				if (comm_oper::makeVector(pnt1, bound2).normalize().isVertical(vec_horizon))
				{
					select_pnt1 = true;
					break;
				}
				if (comm_oper::makeVector(pnt2, bound2).normalize().isVertical(vec_horizon))
				{
					select_pnt2 = true;
					break;
				}
				if (comm_oper::makeVector(intersected_pnt, bound2).normalize().isVertical(vec_horizon))
				{
					select_intersect = true;
					break;
				}
			} while (0);

			if (select_pnt1 && select_pnt2)
			{
				;  // �������ཻ
			} 
			else if (select_pnt1 && select_intersect)
			{
				pnt2 = intersected_pnt;
			}
			else if (select_pnt2 && select_intersect)
			{
				pnt1 = intersected_pnt;
			}
		}
	} 
	else  // count = 2 || count = -2
	{
		//����ƽ��v_rect[i]��4�������ھ���ƽ��v_pntcldpln[j]��һ��
		float pnt1_plane_dist = 0, pnt2_plane_dist = 0;
		pnt1_plane_dist = fabsf(plncoeff.a * pnt1.x + plncoeff.b * pnt1.y + plncoeff.c * pnt1.z + plncoeff.d);
		pnt2_plane_dist = fabsf(plncoeff.a * pnt2.x + plncoeff.b * pnt2.y + plncoeff.c * pnt2.z + plncoeff.d);

		if (pnt1_plane_dist < PNT2PLANE_THRESHOLD || pnt2_plane_dist < PNT2PLANE_THRESHOLD)
		{
			std::cout<<"�߶ε��ӳ�����ƽ���ཻ"<<std::endl;

			//!��ͬһ��ʱ������get_intersected_pnt()һ��Ҫע�⴫�����������˳��
			Point intersected_pnt;

			if (pnt1_plane_dist < pnt2_plane_dist)
			{
				get_intersected_pnt(pnt1, pnt2, plncoeff, intersected_pnt);

				DEBUG_PRINT(pnt1_plane_dist, pnt1, pnt2, intersected_pnt);
				pnt1 = intersected_pnt;
			} 
			else
			{
				get_intersected_pnt(pnt2, pnt1, plncoeff, intersected_pnt);

				DEBUG_PRINT(pnt2_plane_dist, pnt2, pnt1, intersected_pnt);
				pnt2 = intersected_pnt;
			}
		}
	}

	return true;
}

 /** \brief  �����е���ƽ����вü�
    * \param[in]  pntcld_original ԭʼ����
   * \param[in]  v_pntcldpln �����ÿ������ƽ��
   * \param[out]  v_rect �ü�֮���ÿ��ƽ���Ӧ��"���ο�"
   */
static bool cut_all_corner(const PointCloud<PointXYZ>::Ptr pntcld_original, const VecPointCloudPlane & v_pntcldpln, VecRect & v_rect)
{
	v_pcp_size_type plane_count = v_pntcldpln.size();

	for (v_pcp_size_type i = 0; i < plane_count; ++i)
	{
		//pnt1��pnt2���ɵķ���������XOYƽ��ƽ�У�pnt2��pnt3���ɵķ���������XOYƽ�洹ֱ
		Point &pnt1 = v_rect[i].m_verticle1;
		Point &pnt2 = v_rect[i].m_verticle2;
		Point &pnt3 = v_rect[i].m_verticle3;
		Point &pnt4 = v_rect[i].m_verticle4;

		for (v_pcp_size_type j = 0; j < plane_count; ++j)
		{
			if (j == i)
			{
				continue;
			}
			size_t pln_pnt_count = v_pntcldpln[j].mv_pointcloud.size();
			size_t v1_index = 0, v2_index = 0, v3_index = 0;
			do 
			{
#ifndef _DEBUG
				srand((unsigned)time(NULL));
#endif
				v1_index = static_cast<size_t>(rand() % pln_pnt_count);
				v2_index = static_cast<size_t>(rand() % pln_pnt_count);
				v3_index = static_cast<size_t>(rand() % pln_pnt_count);
			} while (v1_index == v2_index || v1_index == v3_index || v2_index == v3_index);

			const Point &verticle1 = v_pntcldpln[j].mv_pointcloud[v1_index];
			const Point &verticle2 = v_pntcldpln[j].mv_pointcloud[v2_index];
			const Point &verticle3 = v_pntcldpln[j].mv_pointcloud[v3_index];
			PlaneCoeff coeff;

			get_planecoeff(verticle1, verticle2, verticle3, coeff);
			get_rect_edge_cut(pntcld_original, v_pntcldpln[i].m_coeff, v_pntcldpln[j].m_coeff, pnt1, pnt2);
			get_rect_edge_cut(pntcld_original, v_pntcldpln[i].m_coeff, v_pntcldpln[j].m_coeff, pnt4, pnt3);
		}
	}

#ifdef _DEBUG
	for (size_t i = 0; i < v_rect.size(); ++i)
	{
		Rect &rect = v_rect[i];
		std::cout<<"-----------���Ϊ"<<i<<"�ľ���"<<std::endl;
		std::cout<<rect.m_verticle1.x<<", "<<rect.m_verticle1.y<<", "<<rect.m_verticle1.z<<"\n"
			<<rect.m_verticle2.x<<", "<<rect.m_verticle2.y<<", "<<rect.m_verticle2.z<<"\n"
			<<rect.m_verticle3.x<<", "<<rect.m_verticle3.y<<", "<<rect.m_verticle3.z<<"\n"
			<<rect.m_verticle4.x<<", "<<rect.m_verticle4.y<<", "<<rect.m_verticle4.z<<std::endl;
		std::cout<<std::endl;
	}
#endif


	return true;
}

 /** \brief  ����ȷ����ƽ��Ķ���
 * \param[in]  pntcld_original ԭʼ����
   * \param[in]  v_pntcldpln PointCloudPlane��ƽ��
   * \param[out]  v_rect ��ƽ������Ӧ�ľ���
   */
bool traverse_determin_planes_verticles(const PointCloud<PointXYZ>::Ptr pntcld_original, /*const*/ VecPointCloudPlane & v_pntcldpln, VecRect & v_rect)
{
	v_pcp_size_type plane_count = v_pntcldpln.size();

	v_rect.resize(plane_count);
	for (v_pcp_size_type i = 0; i < plane_count; ++i)
	{
		determin_plane_verticles(v_pntcldpln[i], v_rect[i]);
	}
#ifdef _CUT
	cut_all_corner(pntcld_original, v_pntcldpln, v_rect);
#endif
	return true;
}

 /** \brief  �Ӿ���4������ȷ��һ������ƽ��
   * \param[in]  rect �����Rect���������ε�4������
   * \param[out]  cld �õ��ĵ���PointCloud
   */
bool determin_plane_from_rect(Rect & rect, PointCloud<PointXYZ>::Ptr cld)
{
	Point &verticle1 = rect.m_verticle1;
	Point &verticle2 = rect.m_verticle2;
	Point &verticle3 = rect.m_verticle3;
	Point &verticle4 = rect.m_verticle4;

	//ע��rect�����4���㲻һ���ʱ�׼���Σ���һ����һ����������ƽ�е��ı���
	Vector vec12(verticle2.x - verticle1.x, verticle2.y - verticle1.y, verticle2.z - verticle1.z);
	Vector vec14(verticle4.x - verticle1.x, verticle4.y - verticle1.y, verticle4.z - verticle1.z);
	Vector vec23(verticle3.x - verticle2.x, verticle3.y - verticle2.y, verticle3.z - verticle2.z);
	Vector vec43(verticle3.x - verticle4.x, verticle3.y - verticle4.y, verticle3.z - verticle4.z);
	float vec12_length = vec12.getVecLength();
	float vec43_length = vec43.getVecLength();
	float vec14_length = vec14.getVecLength();
	float iter12_43 = 250; //!����vec12��vec43����һ������250��
	float iter14 = 250; //!����vec14����һ������250��
	float iter23 = 250; //!����vec23����һ������250��
	float iter_triang_14 = 20; //!����������������vec14����һ������50��

	int i = 0, j = 0;
	for (Point it1 = verticle1; i < iter12_43 + 1; ++i)
	{
		j = 0;
		for (Point it2 = it1; j < iter14 + 1; ++j)
		{
			pcl::PointXYZ basic_point;  

			basic_point.x = it2.x;
			basic_point.y = it2.y;  
			basic_point.z = it2.z;
			cld->points.push_back(basic_point);  

			if (it2 == verticle3)
			{
				Point it3 = Point(verticle3.x - vec23.x / iter23, verticle3.y - vec23.y / iter23, verticle3.z - vec23.z / iter23);
				int k = 0, L = 0;
				for ( ; k < iter23; ++k)
				{
					//���¼�����������ƽ����vec14��������ʹ�䳤�Ⱥ���
					float vec_len = (iter23 - k - 1) / iter23 * vec14_length;
					Vector vec(vec14.x / vec14_length * vec_len, vec14.y / vec14_length * vec_len, vec14.z / vec14_length * vec_len);

					L = 0;
					iter_triang_14 = (iter23 - k - 1) / iter23 * iter14;
					for (Point it4 = it3; L < iter_triang_14 + 1; ++L)
					{
						pcl::PointXYZ basic_point;  

						basic_point.x = it4.x;
						basic_point.y = it4.y;  
						basic_point.z = it4.z;
						cld->points.push_back(basic_point);  

						it4 = Point(it4.x - vec.x / iter_triang_14, it4.y - vec.y / iter_triang_14, it4.z - vec.z / iter_triang_14);
					}
					it3 = Point(it3.x - vec23.x / iter23, it3.y - vec23.y / iter23, it3.z - vec23.z / iter23);
				}
				goto COMPLETE_FILL;
			}
			it2 = Point(it2.x + vec14.x / iter14, it2.y + vec14.y / iter14, it2.z + vec14.z / iter14);
		}
		
		if (it1 == verticle2)
		{
			int k = 0, L = 0;
			for (Point it3 = it1; k < iter23 + 1; ++k)
			{
				//���¼�����������ƽ����vec14��������ʹ�䳤�Ⱥ���
				float vec_len = (iter23 - k) / iter23 * vec14_length;
				Vector vec(vec14.x / vec14_length * vec_len, vec14.y / vec14_length * vec_len, vec14.z / vec14_length * vec_len);

				L = 0;
				iter_triang_14 = (iter23 - k) / iter23 * iter14;
				for (Point it4 = it3; L < iter_triang_14 + 1; ++L)
				{
					pcl::PointXYZ basic_point;  

					basic_point.x = it4.x;
					basic_point.y = it4.y;  
					basic_point.z = it4.z;
					cld->points.push_back(basic_point);  

					it4 = Point(it4.x + vec.x / iter_triang_14, it4.y + vec.y / iter_triang_14, it4.z + vec.z / iter_triang_14);
				}
				it3 = Point(it3.x + vec23.x / iter23, it3.y + vec23.y / iter23, it3.z + vec23.z / iter23);
			}
			goto COMPLETE_FILL;
		}
		it1 = (vec12_length > vec43_length ? Point(it1.x + vec43.x / iter12_43, it1.y + vec43.y / iter12_43, it1.z + vec43.z / iter12_43)
			      : Point(it1.x + vec12.x / iter12_43, it1.y + vec12.y / iter12_43, it1.z + vec12.z / iter12_43));
	}
	
COMPLETE_FILL:
	cld->width = static_cast<unsigned int>(cld->points.size());  
	cld->height = 1;

	return true;
}

 /** \brief  �Ӿ���4������ȷ��һ������ƽ�棬ֻ��������
   * \param[in]  rect �����Rect���������ε�4������
   * \param[out]  cld �õ��ĵ���PointCloud
   */
bool determin_plane_from_rect_only_contour(Rect & rect, PointCloud<PointXYZ>::Ptr cld)
{
	Point &verticle1 = rect.m_verticle1;
	Point &verticle2 = rect.m_verticle2;
	Point &verticle3 = rect.m_verticle3;
	Point &verticle4 = rect.m_verticle4;

	//ע��rect�����4���㲻һ���ʱ�׼���Σ���һ����һ����������ƽ�е��ı���
	Vector vec12(verticle2.x - verticle1.x, verticle2.y - verticle1.y, verticle2.z - verticle1.z);
	Vector vec14(verticle4.x - verticle1.x, verticle4.y - verticle1.y, verticle4.z - verticle1.z);
	Vector vec23(verticle3.x - verticle2.x, verticle3.y - verticle2.y, verticle3.z - verticle2.z);
	Vector vec43(verticle3.x - verticle4.x, verticle3.y - verticle4.y, verticle3.z - verticle4.z);
	float vec12_length = vec12.getVecLength();
	float vec43_length = vec43.getVecLength();
	float vec14_length = vec14.getVecLength();
	float iter12_43 = 250; //!����vec12��vec43����һ������250��
	float iter14 = 250; //!����vec14����һ������250��
	float iter23 = 250; //!����vec23����һ������250��
	float iter_triang_14 = 20; //!����������������vec14����һ������50��

	int i = 0, j = 0;
	for (Point it1 = verticle1; i < iter12_43 + 1; ++i)
	{
		pcl::PointXYZ basic_point;  

		basic_point.x = it1.x;
		basic_point.y = it1.y;  
		basic_point.z = it1.z;
		cld->points.push_back(basic_point);

		it1 = Point(it1.x + vec12.x / iter12_43, it1.y + vec12.y / iter12_43, it1.z + vec12.z / iter12_43);
	}

	i = 0;
	for (Point it1 = verticle2; i < iter23 + 1; ++i)
	{
		pcl::PointXYZ basic_point;  

		basic_point.x = it1.x;
		basic_point.y = it1.y;  
		basic_point.z = it1.z;
		cld->points.push_back(basic_point);

		it1 = Point(it1.x + vec23.x / iter23, it1.y + vec23.y / iter23, it1.z + vec23.z / iter23);
	}

	i = 0;
	for (Point it1 = verticle3; i < iter12_43 + 1; ++i)
	{
		pcl::PointXYZ basic_point;  

		basic_point.x = it1.x;
		basic_point.y = it1.y;  
		basic_point.z = it1.z;
		cld->points.push_back(basic_point);

		it1 = Point(it1.x - vec43.x / iter12_43, it1.y - vec43.y / iter12_43, it1.z - vec43.z / iter12_43);
	}

	//pcl::PointXYZ basic_point1;  

	//basic_point1.x = verticle3.x;
	//basic_point1.y = verticle3.y;  
	//basic_point1.z = verticle3.z;
	//cld->points.push_back(basic_point1);

	//std::cout<<verticle3<<std::endl;

	i = 0;
	for (Point it1 = verticle4; i < iter14 + 1; ++i)
	{
		pcl::PointXYZ basic_point;  

		basic_point.x = it1.x;
		basic_point.y = it1.y;  
		basic_point.z = it1.z;
		cld->points.push_back(basic_point);

		it1 = Point(it1.x - vec14.x / iter12_43, it1.y - vec14.y / iter12_43, it1.z - vec14.z / iter12_43);
	}
	cld->width = static_cast<unsigned int>(cld->points.size());  
	cld->height = 1;

	return true;
}
