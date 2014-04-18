#ifndef _OTHRE_HPP_
#define _OTHRE_HPP_

#include "common.h"
#include "typesdef.h"
#include <iostream>

inline void DEBUG_PRINT(float pnt_plane, Point &pnt1, Point &pnt2, Point &intersected_pnt)
{
#ifdef _DEBUG
	Point temp_pnt = pnt1;
	Vector vec, vec2;

	std::cout<<"ԭʼ������"<<temp_pnt;
	std::cout<<"ԭʼ�㵽ƽ��ľ�����:"<<fabsf(pnt_plane)<<std::endl;

	std::cout<<"��������"<<intersected_pnt;
	vec = Vector(intersected_pnt.x - temp_pnt.x, intersected_pnt.y - temp_pnt.y, intersected_pnt.z - temp_pnt.z);
	vec2= Vector(pnt2.x - temp_pnt.x, pnt2.y - temp_pnt.y, pnt2.z - temp_pnt.z);
	std::cout<<"������ԭʼ���ľ�����:"<<vec.getVecLength()<<std::endl;
	std::cout<<"intersected - pnt������"<<vec<<std::endl;
	std::cout<<"pnt - pnt������"<<vec2<<std::endl;
#endif
}

inline bool get_output_dir(const string &filename, string &dirname)
{
	size_t found = filename.find_last_of("/\\");
	dirname = filename.substr(0,found);
	return true;
}

#endif  //_OTHRE_HPP_
