#include <iostream>
using namespace std;
#ifndef _OL_H_
#define _OL_H_
//ʮ���������ϡ�����
///////////////////////////////////
//����Ԫ�ؽ����(��isHeadΪ��ʱ����ͷ���)
class OrthogonalListNode
{
private:
	bool isHead;//��ʾ�Ƿ�ͷ���
	int col;//�����к�
	int row;//�����к�
	int value;//ֵ
	OrthogonalListNode* right;//ָ��ͬһ���ұ߷���Ԫ��
	OrthogonalListNode* down;//ָ��ͬһ���������Ԫ��
	OrthogonalListNode* next;//����ͷ���
	friend class OrthogonalList;
public:
	//���캯��
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
	};//������ �� ֵ
	//��������
	~OrthogonalListNode()
	{
		
	}
};



//ʮ��������
class OrthogonalList
{
private:
	
	//����˽�к��� ������
	void insert(int i,int j,int n)
	{
		OrthogonalListNode* new_ln = new OrthogonalListNode();
		OrthogonalListNode* temp = headOfAll->next;
		for(int k=0;k<j;++k)
		{
			temp = temp->next;
		}//ѭ�������ҵ���Ӧ�����е�ͷ���
		if(temp->down==temp)//����ֱ������
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
			//�ɲ嵽temp����
		
			new_ln->row = i;
			new_ln->col = j;
			new_ln->value = n;
			new_ln->down = temp->down;
			temp->down = new_ln;
		}//�����������

		//������ָ��
		temp = headOfAll->next;
		for(int k=0;k<i;++k)
		{
			temp = temp->next;
		}//ѭ�������ҵ���Ӧ�����е�ͷ���
		if(temp->right==temp)//����ֱ������
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
			//�ɲ嵽temp����
			new_ln->right = temp->right;
			temp->right = new_ln;
		}//�����������
	};//����λ�ü�ֵ




public:
	OrthogonalListNode* headOfAll;//�����ܾ�����Ϣ�������������ܷ���Ԫ���������������б�ͷ���
	//���캯��
	OrthogonalList()
	{
		headOfAll = new OrthogonalListNode();
	};



	//����ʮ������
	void createOL(int row,int col,int** array)
	{
		headOfAll->row = row;
		headOfAll->col = col;
		headOfAll->next = headOfAll;
		int num = 0;//�������Ԫ���ܸ���
		int headListNum = (row > col ? row:col);//ȡ����ֵ�ϴ���Ϊͷ�������
		for(int i=0;i<headListNum;++i)//ѭ������ͷ���
		{
			//ͷ�巨
			OrthogonalListNode* hn = new OrthogonalListNode(0,0,headListNum-i-1);
			hn->isHead = true;
			hn->down = hn;
			hn->right = hn;
			hn->next = headOfAll->next;
			headOfAll->next = hn;
		}
		//��������
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
	};//����ϡ�������� ��ֵ ����Ӧ����
	//��ӡ����
	void show()
	{
		OrthogonalListNode* temp = headOfAll;
		OrthogonalListNode* temp1;
		if(headOfAll->value == 0)//��
		{
			cout<<"�գ�"<<endl;
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
					//�����Ϣ
					cout<<"("<<temp1->row<<","<<temp1->col<<")"<<">>>>"<<temp1->value<<endl;
				}while(temp1->down!=temp);
			}
		}while(temp->next!=headOfAll);
	};

	//�������
	void add(OrthogonalList ol)
	{
		if(headOfAll->col!=ol.headOfAll->col || headOfAll->row!=ol.headOfAll->row)
		{
			cout<<"����ƥ�䣬�޷���ӣ�"<<endl;
			return;
		}
		//���ж�ӦԪ�ؾ���ӣ����������
		OrthogonalListNode* temp = headOfAll;
		OrthogonalListNode* temp_ol = ol.headOfAll;
		OrthogonalListNode* temp1;
		OrthogonalListNode* temp1_ol;
		if(headOfAll->value == 0)//��
		{
			return;
		}
		do//��������ͷ���
		{
			temp = temp->next;
			temp_ol = temp_ol->next;
			if(temp->down == temp)//���������п�
			{
				if(temp_ol->down != temp_ol)//�������зǿ�
				{
					temp1_ol = temp_ol;
					do
					{
						temp1_ol = temp1_ol->down;
						insert(temp1_ol->row,temp1_ol->col,temp1_ol->value);
					}while(temp1_ol->down!=temp_ol);
				}
			
			}
			else//���������зǿ�
			{
				if(temp_ol->down != temp_ol)//�Ҽ�����Ҳ�ǿ�
				{
					temp1_ol = temp_ol->down;
					temp1 = temp->down;
					do
					{
						if(temp1->row == temp1_ol->row)//��λ��ƥ�� ����ָ�붼��һ
						{	
							if((temp1->value + temp1_ol->value)==0)//�����Ϊ�� ��ɾ���ڵ�
							{
								OrthogonalListNode* pre_rem = temp1;//�������ϼ�
								OrthogonalListNode* remove_me = new OrthogonalListNode();//����ɾ��
								remove_me = temp1;
								while(pre_rem->right!=temp1)//Ѱ�ұ�ɾ�������ϼ�
									pre_rem = pre_rem->right;
								pre_rem->right = temp1->right;
								pre_rem = temp1;
								while(pre_rem->down!=temp1)//Ѱ�ұ�ɾ�������ϼ�
									pre_rem = pre_rem->down;
								pre_rem->down = temp1->down;
								temp1 = pre_rem;
								delete remove_me;
							}
							else
								temp1->value += temp1_ol->value;
							//ָ���һ
							temp1 = temp1->down;
							temp1_ol = temp1_ol->down;
						}
						else//����ƥ�� ֻ�м���ָ���һ
						{
							insert(temp1_ol->row,temp1_ol->col,temp1_ol->value);
							temp1_ol = temp1_ol->down;
						}
					}while(temp1_ol!=temp_ol && temp1!=temp);//���ۼ���������ֻҪ��������˳�
					if(temp1==temp)//�����ڱ�������������˳� �轫����ʣ��ڵ����
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