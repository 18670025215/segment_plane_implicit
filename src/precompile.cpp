// ���ļ��������ڴ���Ԥ����ͷ [4/17/2014 pc]
#include "common.h"
#include "typesdef.h"

//˳�����������������ˣ������ͷ�ļ�typesdef.h���Ҷ��Դ�ļ�����typesdef.h�ᱨ�����ض������
Vector makeVector(const _Point & pnt1, const _Point & pnt2)
{
	return Vector(pnt1.x - pnt2.x, pnt1.y - pnt2.y, pnt1.z - pnt2.z);
}
