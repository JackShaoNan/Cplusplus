#include <iostream>
using namespace std;
#ifndef _OL_H_
#define _OL_H_
//十字链表存贮稀疏矩阵
///////////////////////////////////
//非零元素结点类(当isHead为真时用作头结点)
class OrthogonalListNode
{
private:
	bool isHead;//表示是否头结点
	int col;//所在列号
	int row;//所在行号
	int value;//值
	OrthogonalListNode* right;//指向同一行右边非零元素
	OrthogonalListNode* down;//指向同一列下面非零元素
	OrthogonalListNode* next;//连接头结点
	friend class OrthogonalList;
public:
	//构造函数
	OrthogonalListNode():isHead(false),right(NULL),down(NULL),next(NULL){};
	OrthogonalListNode(int a,int b,int c)
	{
		isHead = false;
		row = a;
		col = b;
		value = c;
		right = NULL;
		down = NULL;
		next = NULL;
	};//传入行 列 值
	//析构函数
	~OrthogonalListNode()
	{
		
	}
};



//十字链表类
class OrthogonalList
{
private:
	
	//辅助私有函数 插入结点
	void insert(int i,int j,int n)
	{
		OrthogonalListNode* new_ln = new OrthogonalListNode();
		OrthogonalListNode* temp = headOfAll->next;
		for(int k=0;k<j;++k)
		{
			temp = temp->next;
		}//循环结束找到对应插入列的头结点
		if(temp->down==temp)//若空直接连接
		{
		
			new_ln->row = i;
			new_ln->col = j;
			new_ln->value = n;
			new_ln->down = temp->down;
			temp->down = new_ln;
		}
		else
		{
			while(temp->down->row < i && temp->down->isHead != true)
			{
				temp = temp->down;
			}
			//可插到temp下面
		
			new_ln->row = i;
			new_ln->col = j;
			new_ln->value = n;
			new_ln->down = temp->down;
			temp->down = new_ln;
		}//列已连接完毕

		//连接行指针
		temp = headOfAll->next;
		for(int k=0;k<i;++k)
		{
			temp = temp->next;
		}//循环结束找到对应插入行的头结点
		if(temp->right==temp)//若空直接连接
		{
			temp->right = new_ln;
			new_ln->right = temp;
		}
		else
		{
			while(temp->right->col < j && temp->right->isHead != true)
			{
				temp = temp->right;
			}
			//可插到temp右面
			new_ln->right = temp->right;
			temp->right = new_ln;
		}//列已连接完毕
	};//插入位置及值




public:
	OrthogonalListNode* headOfAll;//储存总矩阵信息，总行列数，总非零元素数，并连接所有表头结点
	//构造函数
	OrthogonalList()
	{
		headOfAll = new OrthogonalListNode();
	};



	//建立十字链表
	void createOL(int row,int col,int** array)
	{
		headOfAll->row = row;
		headOfAll->col = col;
		headOfAll->next = headOfAll;
		int num = 0;//保存非零元素总个数
		int headListNum = (row > col ? row:col);//取行列值较大者为头结点数量
		for(int i=0;i<headListNum;++i)//循环建立头结点
		{
			//头插法
			OrthogonalListNode* hn = new OrthogonalListNode(0,0,headListNum-i-1);
			hn->isHead = true;
			hn->down = hn;
			hn->right = hn;
			hn->next = headOfAll->next;
			headOfAll->next = hn;
		}
		//遍历矩阵
		for(int i=0;i<row;++i)
		{
			for(int j=0;j<col;++j)
			{
				if(array[i][j] != 0)
				{
					insert(i,j,array[i][j]);
					num++;
				};
			}
		}
		headOfAll->value = num;
	};//传入稀疏矩阵的行 列值 及对应矩阵
	//打印矩阵
	void show()
	{
		OrthogonalListNode* temp = headOfAll;
		OrthogonalListNode* temp1;
		if(headOfAll->value == 0)//空
		{
			cout<<"空！"<<endl;
			return;
		}
		do
		{
			temp = temp->next;
			if(temp->down != temp)
			{
				temp1 = temp;
				do
				{
					temp1 = temp1->down;
					//输出信息
					cout<<"("<<temp1->row<<","<<temp1->col<<")"<<">>>>"<<temp1->value<<endl;
				}while(temp1->down!=temp);
			}
		}while(temp->next!=headOfAll);
	};

	//矩阵相加
	void add(OrthogonalList ol)
	{
		if(headOfAll->col!=ol.headOfAll->col || headOfAll->row!=ol.headOfAll->row)
		{
			cout<<"矩阵不匹配，无法相加！"<<endl;
			return;
		}
		//若有对应元素就相加，若无则插入
		OrthogonalListNode* temp = headOfAll;
		OrthogonalListNode* temp_ol = ol.headOfAll;
		OrthogonalListNode* temp1;
		OrthogonalListNode* temp1_ol;
		if(headOfAll->value == 0)//空
		{
			return;
		}
		do//遍历所有头结点
		{
			temp = temp->next;
			temp_ol = temp_ol->next;
			if(temp->down == temp)//若被加数列空
			{
				if(temp_ol->down != temp_ol)//而加数列非空
				{
					temp1_ol = temp_ol;
					do
					{
						temp1_ol = temp1_ol->down;
						insert(temp1_ol->row,temp1_ol->col,temp1_ol->value);
					}while(temp1_ol->down!=temp_ol);
				}
			
			}
			else//若被加数列非空
			{
				if(temp_ol->down != temp_ol)//且加数列也非空
				{
					temp1_ol = temp_ol->down;
					temp1 = temp->down;
					do
					{
						if(temp1->row == temp1_ol->row)//若位置匹配 则两指针都进一
						{	
							if((temp1->value + temp1_ol->value)==0)//若相加为零 则删除节点
							{
								OrthogonalListNode* pre_rem = temp1;//用于找上家
								OrthogonalListNode* remove_me = new OrthogonalListNode();//用于删除
								remove_me = temp1;
								while(pre_rem->right!=temp1)//寻找被删结点的行上家
									pre_rem = pre_rem->right;
								pre_rem->right = temp1->right;
								pre_rem = temp1;
								while(pre_rem->down!=temp1)//寻找被删结点的列上家
									pre_rem = pre_rem->down;
								pre_rem->down = temp1->down;
								temp1 = pre_rem;
								delete remove_me;
							}
							else
								temp1->value += temp1_ol->value;
							//指针进一
							temp1 = temp1->down;
							temp1_ol = temp1_ol->down;
						}
						else//若不匹配 只有加数指针进一
						{
							insert(temp1_ol->row,temp1_ol->col,temp1_ol->value);
							temp1_ol = temp1_ol->down;
						}
					}while(temp1_ol!=temp_ol && temp1!=temp);//不论加数被加数只要遍历完就退出
					if(temp1==temp)//若由于被加数遍历完而退出 需将加数剩余节点插入
					{
						while(temp1_ol!=temp_ol)
						{
							insert(temp1_ol->row,temp1_ol->col,temp1_ol->value);
							temp1_ol = temp1_ol->down;
						}
					}
				}
			}
		}while(temp->next!=headOfAll);
	};
};







#endif