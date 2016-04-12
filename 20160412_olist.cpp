// 20160412_olist.cpp : 定义控制台应用程序的入口点。
//http://www.cnblogs.com/yangxi/archive/2012/03/22/2411452.html 有关二维数组做参数

#include "stdafx.h"
#include "oList.h"

int _tmain(int argc, _TCHAR* argv[])
{
	int** a = new int*[3];
	int** b = new int*[3];
	for(int i=0;i<3;++i)
	{
		a[i] = new int[3];
		b[i] = new int[3];
	}
	for(int i=0;i<3;++i)
	{
		for(int j=0;j<3;++j)
		{
			a[i][j] = 0;
			b[i][j] = 0;
		}
	};
	a[0][2]=1;
	a[1][1]=2;
	b[0][2]=-1;
	b[1][1]=1;
	b[2][0]=1;
	for(int i=0;i<3;++i)
	{
		cout<<a[i][1]<<endl;
		cout<<b[i][1]<<endl;
	}
	OrthogonalList myol;
	OrthogonalList myol1;
	myol1.createOL(3,3,b);
	myol.createOL(3,3,a);
	myol.show();
	myol1.show();
	myol.add(myol1);
	myol.show();
	system("pause");
	return 0;
}

