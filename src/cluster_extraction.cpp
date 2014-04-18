//http://www.pointclouds.org/documentation/tutorials/extract_indices.php

#include "common.h"
#include "typesdef.h"
#include "process.h"
#include "other.hpp"
#include "meanshift.h"
#include "gaussfilter.h"

string g_str;

int main (int argc, char** argv)
{
	//��Ӳ�̶�ȡpcd��ʽ�ĵ����ļ����������ݺ���RGB��Ϣ
	//pcl::PCDReader reader;
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZ>), cloud_f (new pcl::PointCloud<pcl::PointXYZ>);
	io::loadPCDFile(argv[1], *cloud);
	
	get_output_dir(argv[1], g_str);

	// �²����˲����ڱ��ֵ���������״�����ǰ���£����ٵ������ݣ�����Ҷ�ӳߴ�ֵԽ�󣬵���������Խ��
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_filtered (new pcl::PointCloud<pcl::PointXYZ>);
#pragma region R_VOXELGRID
	//pcl::VoxelGrid<pcl::PointXYZ> vg;
	//vg.setInputCloud (cloud);
	//vg.setLeafSize (0.35f, 0.35f, 0.35f);
	//vg.filter (*cloud_filtered);
	//std::cout << "PointCloud after filtering has: " << cloud_filtered->points.size ()  << " data points." << std::endl; 
#pragma endregion

#pragma region R_GAUSSFILTER
	//GaussFilter gf;
	//gf.setInput(cloud);
	//gf.setRadius(0.8);
	//gf.setSigma(0.4);
	//gf.filter(cloud_filtered);
	//io::savePCDFileASCII("gauss_filtered", *cloud_filtered);
#pragma endregion
	*cloud_filtered = *cloud;

	VecPointIndices v_pnt_indices;//�洢ÿ������ƽ�����ڵ��±�����
	VecPlaneCoeff v_coeff;//�洢����ƽ���ϵ��
	std::vector<VecPoint> v_cloud_plane;//�洢����ƽ��ĵ���
	segment_plane(cloud_filtered, v_pnt_indices, v_coeff, v_cloud_plane, false);

	VecPointCloudPlane v_pntcldplane;
	std::vector<VecPointCloudPlane> v_vec_pntcldplane_clustered;
	build_vec_pointcloudplane(v_cloud_plane, v_coeff, v_pntcldplane);
	cluster_parallel_plane(v_pntcldplane, v_vec_pntcldplane_clustered);

	VecPointCloudPlane v_merged_pntcldplane;
	traverse_merge_planes(v_vec_pntcldplane_clustered, v_merged_pntcldplane);

	int index = 0;
	for (size_t i = 0, cnt = v_merged_pntcldplane.size(); i < cnt; ++i)
	{
		PointCloud<PointXYZ>::Ptr cld(new PointCloud<PointXYZ>());

		pointcloudplane2pointcloud(v_merged_pntcldplane[i], cld);

		std::stringstream ss;
		ss << "merge_" << index << ".pcd";
		index++;

		string str = g_str;
		str += "/";
		str += string(ss.str());
		io::savePCDFileASCII(str, *cld);
	}
	
	VecRect v_rect;
	traverse_determin_planes_verticles(cloud_filtered, v_merged_pntcldplane, v_rect);

	PointCloud<PointXYZ>::Ptr cld_rebuilded(new PointCloud<PointXYZ>());
	PointCloud<PointXYZ>::Ptr cld_contour_rebuilded(new PointCloud<PointXYZ>());
	for (size_t i = 0; i < v_rect.size(); ++i)
	{
		determin_plane_from_rect(v_rect[i], cld_rebuilded);
		determin_plane_from_rect_only_contour(v_rect[i], cld_contour_rebuilded);
	}

	{
		string str, str2;
		str = g_str;

		str2 = (str += "/");
		str += "cloud_rebuiled.pcd";

		io::savePCDFileASCII(str, *cld_rebuilded);
		str2 += "cloud_contour_rebuiled.pcd";
		io::savePCDFileASCII(str2, *cld_contour_rebuilded);
	}

#if defined EUCLIDEAN_CLUSTER_EXTRACTION

	// ���������������󣬾���ָ����
	pcl::search::KdTree<pcl::PointXYZ>::Ptr tree (new pcl::search::KdTree<pcl::PointXYZ>);
	tree->setInputCloud (cloud_filtered);

	std::vector<pcl::PointIndices> cluster_indices;//�������������洢�ָ��ĸ�������
	pcl::EuclideanClusterExtraction<pcl::PointXYZ> ec;//����ָ����
	ec.setClusterTolerance (0.1); // 2cm����ClusterTolerance����ֵΪ���������뾶
	ec.setMinClusterSize (500);//�����е����������
	ec.setMaxClusterSize (25000);//�����е����������
	ec.setSearchMethod (tree);//����������radiusSearch
	ec.setInputCloud ( cloud_filtered);
	ec.extract (cluster_indices);//��ȡ�����뾶�ڵĽ���������Ϊ����ָ���ľ���

	int j = 0;

	//��ʾ�����ָ��ľ���
	for (std::vector<pcl::PointIndices>::const_iterator it = cluster_indices.begin (); it != cluster_indices.end (); ++it)//������ѯ��������cluster_indices�����洢�ľ���
	{
		pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_cluster (new pcl::PointCloud<pcl::PointXYZ>);
		for (std::vector<int>::const_iterator pit = it->indices.begin (); pit != it->indices.end (); pit++)//��cluster_indices�е�pit������ĵ�������д��cloud_cluster
			cloud_cluster->points.push_back (cloud_filtered->points[*pit]); //*
		cloud_cluster->width = cloud_cluster->points.size ();
		cloud_cluster->height = 1;
		cloud_cluster->is_dense = true;

		std::cout << "PointCloud representing the Cluster: " << cloud_cluster->points.size () << " data points." << std::endl;
		std::stringstream ss;
		ss << "cloud_cluster_" << j << ".pcd";
		writer.write<pcl::PointXYZ> (ss.str (), *cloud_cluster, false); //����������ļ�д�뵱ǰĿ¼��
		 j++;
		pcl::visualization::PCLVisualizer viewer("Cloud Viewer"); //������ʾ��pit������
		//pcl::visualization::PointCloudColorHandlerRGBField<pcl::PointXYZ> color(cloud_cluster); //��ʾ����RGB��Ϣ
		viewer.addPointCloud(cloud_cluster,"Cloud Viewer", 0);
		viewer.setBackgroundColor(1,1,1);

		while (!viewer.wasStopped ())
		{
		//?��?????����?����?��??????����
		 viewer.spinOnce();
		}
	}
#endif //EUCLIDEAN_CLUSTER_EXTRACTION

	system("pause");
	return (0);
}
