//文件名不支持中文
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void my_print(char c);
void my_print_file(char c);

typedef unsigned char u8;//一个字节
typedef unsigned short u16;//两个字节
typedef unsigned int u32;//四个字节

//全局变量
int BytesPerSec;//每扇区字节数
int SecPerClus;//每簇扇区数
int RsvdSecCnt;//boot记录占用扇区数
int NumFATs;//FAT表数量
int RootEntCnt;//根目录最大文件数
int FATSz;//每个FAT表包含扇区数

int FileNum;
int DirNum;
char DirName[1000];
u16 FstClus[100];




//定义BPB结构体
#pragma pack (1)  //为节省内存 指定按一个字节对齐

//BPB在引导扇区中偏移11个字节开始，总长25个字节
struct BPB
{
    u16 BPB_BytesPerSec;//每扇区字节数
    u8 BPB_SecPerClus;//每簇扇区数
    u16 BPB_RsvdSecCnt;//boot记录所占扇区数
    u8 BPB_NumFATs;//fat表数量
    u16 BPB_RootEntCnt;//根目录最大文件数
    u16 BPB_TotSec16;//扇区总数
    u8 BPB_Media;
    u16 BPB_FATSz16;//每个fat表包含扇区数
    u16 BPB_SecPerTrk;
    u16 BPB_NumHeads;
    u32 BPB_HiddSec;
    u32 BPB_TotSec32;//若TolSec16装不下，用这个表示扇区总数
};

//根目录条目 总长32字节
struct RootEntry
{
    char DIR_Name[11];//长度为11字节的文件名
    u8 DIR_Attr;//文件属性
    char Reserved[10];
    u16 DIR_WrtTime;
    u16 DIR_WrtDate;
    u16 DIR_FstClus;//开始簇号
    u32 DIR_FileSize;
};

#pragma pack ()//取消指定对齐格式 恢复默认对齐格式

//函数声明
void loadBPB(FILE* fat12,struct BPB* bpb_ptr);//载入BPB
void printFiles(FILE* fat12,struct RootEntry* rootEntry_ptr);//打印文件名
void printChildrenFile(FILE* fat12,char* directory,int startClus);//打印子目录文件（涉及递归）传入目录名及目录开始簇号
int getFATValue(FILE* fat12,int clusNum);//由簇号得到fat表中对应簇的值

void findFileAndPrint(FILE* fat12,char* yourName, struct RootEntry* rootEntry_ptr);//寻找文件名是否存在 并输出相关信息
int findFileInChildrenFile(FILE* fat12,char* directory,int startClus,char* yourName);//看子目录名是否匹配
void printFileContent(FILE* fat12,int startClus);//打印文件内容

void findFileAndDirNum(FILE* fat12,char* yourName,struct RootEntry* rootEntry_ptr);//返回匹配目录下有多少文件和子目录
int findFileAndDirNumInChild(FILE* fat12,char* directory,int startClus,char* yourName);
void fileAndDirNumInDir(FILE* fat12,int startClus);//得到目录下共有多少文件和目录
void fileAndDirNumInChildDir(FILE* fat12,int startClus);//子目录下有多少文件和目录

void findChildName(FILE* fat12,int startClus);//得到所有子目录名字 存放在全局变量中
void findChildNameInChild(FILE* fat12,int startClus);
void findChildFstClus(FILE* fat12,int startClus);//得到所有子目录的开始簇号 存放在全局变量中
void findChildFstClusInChild(FILE* fat12,int startClus);

//主函数
int main()
{
    FILE* fat12;
    fat12 = fopen("abc.img","rb");//打开fat12的镜像文件

    //建立BPB结构及指针
    struct BPB bpb;
    struct BPB* bpb_ptr = &bpb;

    //载入BPB
    loadBPB(fat12,bpb_ptr);

    //初始化各个全局变量
    BytesPerSec = bpb_ptr->BPB_BytesPerSec;
    SecPerClus = bpb_ptr->BPB_SecPerClus;
    RsvdSecCnt = bpb_ptr->BPB_RsvdSecCnt;
    NumFATs = bpb_ptr->BPB_NumFATs;
    RootEntCnt = bpb_ptr->BPB_RootEntCnt;
    FATSz = bpb_ptr->BPB_FATSz16;



    //建立根目录条目及指针
    struct RootEntry rootEntry;
    struct RootEntry* rootEntry_ptr = &rootEntry;

    //打印文件名
    printFiles(fat12,rootEntry_ptr);


    char x[2];
    printf("请选择要测试的功能（a/b）：");
    scanf("%c",x);
    if(x[0] == 'a')
    {
        while(x[0] == 'a'){
            printf("请输入指令（输入分号退出）：\n");
            char path[1000];
            scanf("%s", path);
            if(path[0] == ';'){
                break;
            }
            findFileAndPrint(fat12, path, rootEntry_ptr);
        }
    }
    else
    {
        while(x[0] == 'b'){
            printf("请输入指令（格式为：\"count/目录名\"，同样的分号退出）：\n");
            char path[1000];
            char path1[1000];
            scanf("%s", path);

            if(path[0] == ';'){
                break;
            }

            strcpy(path1,path+6);
            //printf("%s\n",path1);
            //printf("%s\n",path);
            findFileAndDirNum(fat12, path1, rootEntry_ptr);
        }
    }



    //关闭文件
    fclose(fat12);
};

//载入BPB
void loadBPB(FILE* fat12 , struct BPB* bpb_ptr)
{
    int check;

    //BPB从偏移11个字节处开始
    check = fseek(fat12,11,SEEK_SET);
    if (check == -1)
        printf("fseek in fillBPB failed!");

    //BPB长度为25字节
    check = fread(bpb_ptr,1,25,fat12);
    if (check != 25)
        printf("fread in fillBPB failed!");
}


//打印文件名 printFiles（）实现
void printFiles(FILE* fat12,struct RootEntry* rootEntry_ptr)
{
    //首先为跳转到根目录区做准备：找到偏移量
    int base = (RsvdSecCnt+NumFATs*FATSz)*BytesPerSec;//跨过引导扇区和两个FAT表到达根目录区.
    int flag_jump;//标志位：是否跳转成功或是否读入成功
    char realName[13];//保存将要输出的文件名

    //遍历处理根目录区中的各个条目
    int i;
    for(i=0;i<RootEntCnt;++i)
    {
        //首先跳转
        flag_jump = fseek(fat12,base,0);
        if(flag_jump == -1)
            printf("fseek in printFiles failed!");
        flag_jump = fread(rootEntry_ptr,1,32,fat12);//将根目录区的一条记录读入
        if(flag_jump != 32)
            printf("fread in printFiles failed!");
        //为遍历下一条记录做准备
        base += 32;

        //处理记录
        if(rootEntry_ptr->DIR_Name[0] == '\0')
            continue;//名字为空,空条目不输出

        //若非空 判断名字的有效性 看是否只包含英文数字和空格
        int flag_name_valid = 1;//标志位:判断名字是否有效
        int j;
        for(j=0;j<11;++j)
        {
            //遍历每一个字符
            if (!(((rootEntry_ptr->DIR_Name[j] >= 48)&&(rootEntry_ptr->DIR_Name[j] <= 57)) ||
                  ((rootEntry_ptr->DIR_Name[j] >= 65)&&(rootEntry_ptr->DIR_Name[j] <= 90)) ||
                  ((rootEntry_ptr->DIR_Name[j] >= 97)&&(rootEntry_ptr->DIR_Name[j] <= 122)) ||
                  (rootEntry_ptr->DIR_Name[j] == ' ')))
            {
                flag_name_valid = 0;	//非英文及数字、空格
                break;
            }
        }
        if(!flag_name_valid)
            continue;//若由于名字非法结束遍历,则跳过这个条目

        //若名字合法 判断文件类型 准备输出
        if(rootEntry_ptr->DIR_Attr == 0x20)
        {
            //这是一个文件
            int realNameIndex = -1;
            int k;
            for(k=0;k<11;++k)//遍历名字中每个字符,若遇到一个或多个连续空格,以'.'替代之
            {
                if(rootEntry_ptr->DIR_Name[k] != ' ')
                {
                    realNameIndex++;
                    realName[realNameIndex] = rootEntry_ptr->DIR_Name[k];
                }
                else
                {
                    realNameIndex++;
                    realName[realNameIndex] = '.';
                    while(rootEntry_ptr->DIR_Name[k] == ' ')
                        k++;
                    k--;
                }
            }
            realNameIndex++;
            realName[realNameIndex] = '\0';//文件名已存放到realName中 可以输出

            //输出
            strcat(realName,"\n");
            int m;
            for(m=0;m<strlen(realName);++m)
            {
                my_print(realName[m]);
            }
        }
        else
        {
            //这是一个目录 同样先把目录名字保存到realName中
            int realNameIndex = -1;
            int k;
            for(k=0;k<11;++k)//遍历名字中每个字符,若遇到一个或多个连续空格,以'.'替代之
            {
                if(rootEntry_ptr->DIR_Name[k] != ' ')
                {
                    realNameIndex++;
                    realName[realNameIndex] = rootEntry_ptr->DIR_Name[k];
                }
                else
                {
                    realNameIndex++;
                    realName[realNameIndex] = '\0';
                    break;
                }
            }
            realNameIndex++;
            realName[realNameIndex] = '\0';//目录名已存放到realName中 可以输出

            //输出目录名及其中内容
            printChildrenFile(fat12,realName,rootEntry_ptr->DIR_FstClus);
        }
    }
};

//打印目录名及其中内容
void printChildrenFile(FILE* fat12,char* directory,int startClus)
{
    //将全部路径保存 (若这是个空目录最后再输出)
    char dirName[100] = {0};
    strcpy(dirName,directory);
    dirName[strlen(directory)] = '/';
    char* fatherName = &dirName[strlen(dirName)];

    int flag_null_dir = 0;//标志位:若保持0说明是空目录

    //准备访问数据区对应簇号的簇

    int currentClus = startClus;
    int clusValue = 0;//fat表中对应簇的值

    //循环遍历(目录信息可能存在与多个簇)
    while(clusValue < 0xFF8)
    {
        clusValue = getFATValue(fat12,currentClus);//获得当前值

        if(clusValue == 0xFF7) //坏簇直接退出循环
        {
            printf("坏簇,读取失败!\n");
            break;
        }

        //读取成功,访问数据区对应簇

        int dataBase = BytesPerSec * ( RsvdSecCnt + FATSz*NumFATs + (RootEntCnt*32 + BytesPerSec - 1)/BytesPerSec);
        int startByte = dataBase + (currentClus - 2)*SecPerClus*BytesPerSec;


/*      int flag_jump_data;
        flag_jump_data = fseek(fat12,startByte,0);
        if(flag_jump_data == -1)
            printf("fseek in printChildrenFile failed!");

        char* str = (char* )malloc(SecPerClus*BytesPerSec);	//暂存从簇中读出的数据
        char* content = str;

        flag_jump_data = fread(content,1,SecPerClus*BytesPerSec,fat12);
        if(flag_jump_data != SecPerClus*BytesPerSec)
            printf("fread AAAAAAAAAAAAAAin printChildrenFile failed!");
*/
        //现在content相当于一个"根目录区",我们要对其中的条目进行遍历
        int i = 0;
        while(i<SecPerClus*BytesPerSec)
        {


            //建立根目录条目结构体
            struct RootEntry rootEntry;
            struct RootEntry* rootEntry_ptr = &rootEntry;
            int flag_jump_root;
            flag_jump_root = fseek(fat12,startByte,0);
            if(flag_jump_root == -1)
                printf("fseek in printChildrenFile failed!");
            flag_jump_root = fread(rootEntry_ptr,1,32,fat12);
            if(flag_jump_root != 32)
                printf("fread BBBBBBBBBBBBBBBBin printChildrenFile failed!");

            //读取完毕 为下次循环做准备
            i += 32;
            startByte += 32;

            if(rootEntry_ptr->DIR_Name[0] == '\0')
            {   continue;}

            int flag_name_valid = 1;
            int j;
            for (j=0;j<11;j++)
            {
                if (!(((rootEntry_ptr->DIR_Name[j] >= 48)&&(rootEntry_ptr->DIR_Name[j] <= 57)) ||
                      ((rootEntry_ptr->DIR_Name[j] >= 65)&&(rootEntry_ptr->DIR_Name[j] <= 90)) ||
                      ((rootEntry_ptr->DIR_Name[j] >= 97)&&(rootEntry_ptr->DIR_Name[j] <= 122)) ||
                      (rootEntry_ptr->DIR_Name[j] == ' ')))
                {
                    flag_name_valid = 0;	//非英文及数字、空格
                    break;
                }
            }

            if(flag_name_valid == 0)
                continue;

            //名字合法就输出
            char tempName[13];//储存名字
            if(rootEntry_ptr->DIR_Attr == 0x20)
            {
                //这是一个文件
                int realNameIndex = -1;
                int k;
                for(k=0;k<11;++k)//遍历名字中每个字符,若遇到一个或多个连续空格,以'.'替代之
                {
                    if(rootEntry_ptr->DIR_Name[k] != ' ')
                    {
                        realNameIndex++;
                        tempName[realNameIndex] = rootEntry_ptr->DIR_Name[k];
                    }
                    else
                    {
                        realNameIndex++;
                        tempName[realNameIndex] = '.';
                        while(rootEntry_ptr->DIR_Name[k] == ' ')
                            k++;
                        k--;
                    }
                }
                realNameIndex++;
                tempName[realNameIndex] = '\0';//文件名已存放到tempName中 可以输出

                //输出 先输出父目录

                int n;
                for(n=0;n<strlen(dirName);++n)
                    my_print_file(dirName[n]);
                strcpy(fatherName,tempName);
                for(n=0;n<strlen(tempName);++n)
                    my_print(tempName[n]);
                printf("\n");
            }
            else
            {
                //这是一个目录 同样先把目录名字保存到tempName中
                int realNameIndex = -1;
                int k;
                for(k=0;k<11;++k)//遍历名字中每个字符
                {
                    if(rootEntry_ptr->DIR_Name[k] != ' ')
                    {
                        realNameIndex++;
                        tempName[realNameIndex] = rootEntry_ptr->DIR_Name[k];
                    }
                    else
                    {
                        realNameIndex++;
                        tempName[realNameIndex] = '\0';
                        break;
                    }
                }//目录名已存放到tempName中

                //将子目录名接到父目录名后面
                strcpy(fatherName,tempName);

                //输出父子目录名及其中内容
                printChildrenFile(fat12,dirName,rootEntry_ptr->DIR_FstClus);
            }

            //程序执行到这里说明不是空目录
            flag_null_dir = 1;


        };


        currentClus = clusValue;//为寻找下个簇做准备

    }
    if(flag_null_dir == 0)
    {
        int i;
        for(i=0;i<strlen(dirName);++i)
            my_print_file(dirName[i]);
        printf("\n");
    }
};

//获得簇号对应的fat表项值
int getFATValue(FILE* fat12,int clusNum)
{
    //先找到fat1表位置
    int base = RsvdSecCnt * BytesPerSec;
    int startByte = base + clusNum*3/2;
    //读取相应簇
    int flag_jump;
    flag_jump = fseek(fat12,startByte,0);
    if(flag_jump == -1)
        printf("fseek in getFATValue failed!");

    u16 data;
    u16* data_ptr = &data;
    flag_jump = fread(data_ptr,1,2,fat12);
    if(flag_jump != 2)
        printf("fread in getFATValue failed!");

    //判断取这16位的前八位还是后八位
    if(clusNum % 2 == 0)//簇号偶数
        return data<<4;
    else
        return data>>4;

};

//寻找文件名是否存在
void findFileAndPrint(FILE* fat12,char* yourName,struct RootEntry* rootEntry_ptr)
{
    int flag = 0;//标志位:标志是否找到匹配
    //首先为跳转到根目录区做准备：找到偏移量
    int base = (RsvdSecCnt+NumFATs*FATSz)*BytesPerSec;//跨过引导扇区和两个FAT表到达根目录区.
    int flag_jump;//标志位：是否跳转成功或是否读入成功
    char realName[13];//保存将要输出的文件名

    //遍历处理根目录区中的各个条目
    int i;
    for(i=0;i<RootEntCnt;++i)
    {
        //首先跳转
        flag_jump = fseek(fat12,base,0);
        if(flag_jump == -1)
            printf("fseek in printFiles failed!");
        flag_jump = fread(rootEntry_ptr,1,32,fat12);//将根目录区的一条记录读入
        if(flag_jump != 32)
            printf("fread in printFiles failed!");
        //为遍历下一条记录做准备
        base += 32;

        //处理记录
        if(rootEntry_ptr->DIR_Name[0] == '\0')
            continue;//名字为空,空条目不输出

        //若非空 判断名字的有效性 看是否只包含英文数字和空格
        int flag_name_valid = 1;//标志位:判断名字是否有效
        int j;
        for(j=0;j<11;++j)
        {
            //遍历每一个字符
            if (!(((rootEntry_ptr->DIR_Name[j] >= 48)&&(rootEntry_ptr->DIR_Name[j] <= 57)) ||
                  ((rootEntry_ptr->DIR_Name[j] >= 65)&&(rootEntry_ptr->DIR_Name[j] <= 90)) ||
                  ((rootEntry_ptr->DIR_Name[j] >= 97)&&(rootEntry_ptr->DIR_Name[j] <= 122)) ||
                  (rootEntry_ptr->DIR_Name[j] == ' ')))
            {
                flag_name_valid = 0;	//非英文及数字、空格
                break;
            }
        }
        if(!flag_name_valid)
            continue;//若由于名字非法结束遍历,则跳过这个条目

        //若名字合法 判断文件类型 准备输出
        if(rootEntry_ptr->DIR_Attr == 0x20)
        {
            //这是一个文件
            int realNameIndex = -1;
            int k;
            for(k=0;k<11;++k)//遍历名字中每个字符,若遇到一个或多个连续空格,以'.'替代之
            {
                if(rootEntry_ptr->DIR_Name[k] != ' ')
                {
                    realNameIndex++;
                    realName[realNameIndex] = rootEntry_ptr->DIR_Name[k];
                }
                else
                {
                    realNameIndex++;
                    realName[realNameIndex] = '.';
                    while(rootEntry_ptr->DIR_Name[k] == ' ')
                        k++;
                    k--;
                }
            }
            realNameIndex++;
            realName[realNameIndex] = '\0';//文件名已存放到realName中 可以输出

            //比较
            if(strcmp(realName,yourName) == 0) //比较成功 输出文件信息  立刻返回
            {
                printFileContent(fat12,rootEntry_ptr->DIR_FstClus);
                flag = 1;
                return;
            }

            //比较不成功 继续循环

        }
        else
        {
            //这是一个目录 同样先把目录名字保存到realName中
            int realNameIndex = -1;
            int k;
            for(k=0;k<11;++k)//遍历名字中每个字符,若遇到一个或多个连续空格,以'.'替代之
            {
                if(rootEntry_ptr->DIR_Name[k] != ' ')
                {
                    realNameIndex++;
                    realName[realNameIndex] = rootEntry_ptr->DIR_Name[k];
                }
                else
                {
                    realNameIndex++;
                    realName[realNameIndex] = '\0';
                    break;
                }
            }
            realNameIndex++;
            realName[realNameIndex] = '\0';//目录名已存放到realName中 可以输出

            if(strcmp(realName,yourName) == 0)//如果匹配 就输出其下面所有文件和目录
            {
                flag = 1;
                printChildrenFile(fat12,realName,rootEntry_ptr->DIR_FstClus);
                return;
            }
            else
            {
                //若不匹配 判断下由于什么原因不匹配
                if(strncmp(realName,yourName,strlen(realName)) == 0)//若头部已经匹配,则继续比较后续
                {
                    flag = findFileInChildrenFile(fat12,realName,rootEntry_ptr->DIR_FstClus,yourName);
                    if(flag == 1)
                        return;
                }
                //否则 头部就不匹配 无需比较后续
            }
        }
    }
    if(flag == 0)
        printf("文件/目录 不存在!\n");
};

//头部匹配情况下看子目录下是否匹配
int findFileInChildrenFile(FILE* fat12,char* directory,int startClus,char* yourName)
{
    int flag = 0;
    //将全部路径保存 (若这是个空目录最后再输出)
    char dirName[100] = {0};
    strcpy(dirName,directory);
    dirName[strlen(directory)] = '/';
    char* fatherName = &dirName[strlen(dirName)];

    int flag_null_dir = 0;//标志位:若保持0说明是空目录

    //准备访问数据区对应簇号的簇

    int currentClus = startClus;
    int clusValue = 0;//fat表中对应簇的值

    //循环遍历(目录信息可能存在与多个簇)
    while(clusValue < 0xFF8)
    {
        clusValue = getFATValue(fat12,currentClus);//获得当前值

        if(clusValue == 0xFF7) //坏簇直接退出循环
        {
            printf("坏簇,读取失败!\n");
            break;
        }

        //读取成功,访问数据区对应簇

        int dataBase = BytesPerSec * ( RsvdSecCnt + FATSz*NumFATs + (RootEntCnt*32 + BytesPerSec - 1)/BytesPerSec);
        int startByte = dataBase + (currentClus - 2)*SecPerClus*BytesPerSec;


/*      int flag_jump_data;
        flag_jump_data = fseek(fat12,startByte,0);
        if(flag_jump_data == -1)
            printf("fseek in printChildrenFile failed!");

        char* str = (char* )malloc(SecPerClus*BytesPerSec);	//暂存从簇中读出的数据
        char* content = str;

        flag_jump_data = fread(content,1,SecPerClus*BytesPerSec,fat12);
        if(flag_jump_data != SecPerClus*BytesPerSec)
            printf("fread AAAAAAAAAAAAAAin printChildrenFile failed!");
*/
        //现在content相当于一个"根目录区",我们要对其中的条目进行遍历
        int i = 0;
        while(i<SecPerClus*BytesPerSec)
        {


            //建立根目录条目结构体
            struct RootEntry rootEntry;
            struct RootEntry* rootEntry_ptr = &rootEntry;
            int flag_jump_root;
            flag_jump_root = fseek(fat12,startByte,0);
            if(flag_jump_root == -1)
                printf("fseek in printChildrenFile failed!");
            flag_jump_root = fread(rootEntry_ptr,1,32,fat12);
            if(flag_jump_root != 32)
                printf("fread BBBBBBBBBBBBBBBBin printChildrenFile failed!");

            //读取完毕 为下次循环做准备
            i += 32;
            startByte += 32;

            if(rootEntry_ptr->DIR_Name[0] == '\0')
            {   continue;}

            int flag_name_valid = 1;
            int j;
            for (j=0;j<11;j++)
            {
                if (!(((rootEntry_ptr->DIR_Name[j] >= 48)&&(rootEntry_ptr->DIR_Name[j] <= 57)) ||
                      ((rootEntry_ptr->DIR_Name[j] >= 65)&&(rootEntry_ptr->DIR_Name[j] <= 90)) ||
                      ((rootEntry_ptr->DIR_Name[j] >= 97)&&(rootEntry_ptr->DIR_Name[j] <= 122)) ||
                      (rootEntry_ptr->DIR_Name[j] == ' ')))
                {
                    flag_name_valid = 0;	//非英文及数字、空格
                    break;
                }
            }

            if(flag_name_valid == 0)
                continue;

            //名字合法就输出
            char tempName[13];//储存名字
            if(rootEntry_ptr->DIR_Attr == 0x20)
            {
                //这是一个文件
                int realNameIndex = -1;
                int k;
                for(k=0;k<11;++k)//遍历名字中每个字符,若遇到一个或多个连续空格,以'.'替代之
                {
                    if(rootEntry_ptr->DIR_Name[k] != ' ')
                    {
                        realNameIndex++;
                        tempName[realNameIndex] = rootEntry_ptr->DIR_Name[k];
                    }
                    else
                    {
                        realNameIndex++;
                        tempName[realNameIndex] = '.';
                        while(rootEntry_ptr->DIR_Name[k] == ' ')
                            k++;
                        k--;
                    }
                }
                realNameIndex++;
                tempName[realNameIndex] = '\0';//文件名已存放到tempName中 可以输出

                //比较输出 先输出父目录

                strcpy(fatherName,tempName);
                if(strcmp(dirName,yourName) == 0)
                {
                    printFileContent(fat12,rootEntry_ptr->DIR_FstClus);
                    return 1;
                }
            }
            else
            {
                //这是一个目录 同样先把目录名字保存到tempName中
                int realNameIndex = -1;
                int k;
                for(k=0;k<11;++k)//遍历名字中每个字符
                {
                    if(rootEntry_ptr->DIR_Name[k] != ' ')
                    {
                        realNameIndex++;
                        tempName[realNameIndex] = rootEntry_ptr->DIR_Name[k];
                    }
                    else
                    {
                        realNameIndex++;
                        tempName[realNameIndex] = '\0';
                        break;
                    }
                }//目录名已存放到tempName中

                //将子目录名接到父目录名后面
                strcpy(fatherName,tempName);

                if(strcmp(dirName,yourName) == 0)
                {
                    printChildrenFile(fat12,dirName,rootEntry_ptr->DIR_FstClus);
                    flag = 1;
                    return 1;
                }
                else
                {
                    if(strncmp(dirName,yourName,strlen(dirName)) == 0)
                    {

                        flag = findFileInChildrenFile(fat12,dirName,rootEntry_ptr->DIR_FstClus,yourName);
                        if(flag == 1)
                            return 1;
                    }
                }
            }
        };


        currentClus = clusValue;//为寻找下个簇做准备
    }

    printf("文件/目录 不存在!\n");
    return 0;
};

//打印文件内容
void printFileContent(FILE* fat12,int startClus)
{
    //准备访问数据区对应簇号的簇

    int currentClus = startClus;
    int clusValue = 0;//fat表中对应簇的值

    //循环遍历(目录信息可能存在与多个簇)
    while(clusValue < 0xFF8)
    {
        clusValue = getFATValue(fat12, currentClus);//获得当前值

        if (clusValue == 0xFF7) //坏簇直接退出循环
        {
            printf("坏簇,读取失败!\n");
            break;
        }

        //读取成功,访问数据区对应簇

        int dataBase = BytesPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytesPerSec - 1) / BytesPerSec);
        int startByte = dataBase + (currentClus - 2) * SecPerClus * BytesPerSec;


        int flag_jump_data;
        flag_jump_data = fseek(fat12,startByte,0);
        if(flag_jump_data == -1)
            printf("fseek in printChildrenFile failed!");

        char* str = (char* )malloc(SecPerClus*BytesPerSec);	//暂存从簇中读出的数据
        char* content = str;

        flag_jump_data = fread(content,1,SecPerClus*BytesPerSec,fat12);
        if(flag_jump_data != SecPerClus*BytesPerSec)
            printf("fread AAAAAAAAAAAAAAin printChildrenFile failed!");

        //输出
        int i;
        for(i=0;i<strlen(content);++i)
            my_print(content[i]);
        //my_print("\n");

        currentClus = clusValue;
    }
    printf("\n");
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void findChildName(FILE* fat12,int startClus)//得到所有子目录名字 存放在全局变量中
{



    //准备访问数据区对应簇号的簇

    int currentClus = startClus;
    int clusValue = 0;//fat表中对应簇的值

    //循环遍历(目录信息可能存在与多个簇)
    while(clusValue < 0xFF8)
    {
        clusValue = getFATValue(fat12,currentClus);//获得当前值

        if(clusValue == 0xFF7) //坏簇直接退出循环
        {
            printf("坏簇,读取失败!\n");
            break;
        }

        //读取成功,访问数据区对应簇

        int dataBase = BytesPerSec * ( RsvdSecCnt + FATSz*NumFATs + (RootEntCnt*32 + BytesPerSec - 1)/BytesPerSec);
        int startByte = dataBase + (currentClus - 2)*SecPerClus*BytesPerSec;


/*      int flag_jump_data;
        flag_jump_data = fseek(fat12,startByte,0);
        if(flag_jump_data == -1)
            printf("fseek in printChildrenFile failed!");

        char* str = (char* )malloc(SecPerClus*BytesPerSec);	//暂存从簇中读出的数据
        char* content = str;

        flag_jump_data = fread(content,1,SecPerClus*BytesPerSec,fat12);
        if(flag_jump_data != SecPerClus*BytesPerSec)
            printf("fread AAAAAAAAAAAAAAin printChildrenFile failed!");
*/
        //现在content相当于一个"根目录区",我们要对其中的条目进行遍历
        int i = 0;
        while(i<SecPerClus*BytesPerSec)
        {


            //建立根目录条目结构体
            struct RootEntry rootEntry;
            struct RootEntry* rootEntry_ptr = &rootEntry;
            int flag_jump_root;
            flag_jump_root = fseek(fat12,startByte,0);
            if(flag_jump_root == -1)
                printf("fseek in printChildrenFile failed!");
            flag_jump_root = fread(rootEntry_ptr,1,32,fat12);
            if(flag_jump_root != 32)
                printf("fread BBBBBBBBBBBBBBBBin printChildrenFile failed!");

            //读取完毕 为下次循环做准备
            i += 32;
            startByte += 32;

            if(rootEntry_ptr->DIR_Name[0] == '\0')
            {   continue;}

            int flag_name_valid = 1;
            int j;
            for (j=0;j<11;j++)
            {
                if (!(((rootEntry_ptr->DIR_Name[j] >= 48)&&(rootEntry_ptr->DIR_Name[j] <= 57)) ||
                      ((rootEntry_ptr->DIR_Name[j] >= 65)&&(rootEntry_ptr->DIR_Name[j] <= 90)) ||
                      ((rootEntry_ptr->DIR_Name[j] >= 97)&&(rootEntry_ptr->DIR_Name[j] <= 122)) ||
                      (rootEntry_ptr->DIR_Name[j] == ' ')))
                {
                    flag_name_valid = 0;	//非英文及数字、空格
                    break;
                }
            }

            if(flag_name_valid == 0)
                continue;

            //名字合法就输出

            if(rootEntry_ptr->DIR_Attr == 0x20)
            {
                //这是一个文件


            }
            else
            {
                char tempName[13];//储存名字
                int realNameIndex = -1;
                int k;
                for(k=0;k<11;++k)//遍历名字中每个字符
                {
                    if(rootEntry_ptr->DIR_Name[k] != ' ')
                    {
                        realNameIndex++;
                        tempName[realNameIndex] = rootEntry_ptr->DIR_Name[k];
                    }
                    else
                    {
                        realNameIndex++;
                        tempName[realNameIndex] = '\0';
                        break;
                    }
                }//目录名已存放到tempName中
                //这是一个目录
                  int i=0;
                  while(DirName[i] != '\0' && DirName[i] != '/')
                      i++;
                  if(DirName[i] == '/')
                      i++;
                  strcpy(&DirName[i],tempName);
                  DirName[strlen(DirName)] = '/';

                findChildNameInChild(fat12,rootEntry_ptr->DIR_FstClus);
            }




        };


        currentClus = clusValue;//为寻找下个簇做准备

    }


};
void findChildNameInChild(FILE* fat12,int startClus)
{



    //准备访问数据区对应簇号的簇

    int currentClus = startClus;
    int clusValue = 0;//fat表中对应簇的值

    //循环遍历(目录信息可能存在与多个簇)
    while(clusValue < 0xFF8)
    {
        clusValue = getFATValue(fat12,currentClus);//获得当前值

        if(clusValue == 0xFF7) //坏簇直接退出循环
        {
            printf("坏簇,读取失败!\n");
            break;
        }

        //读取成功,访问数据区对应簇

        int dataBase = BytesPerSec * ( RsvdSecCnt + FATSz*NumFATs + (RootEntCnt*32 + BytesPerSec - 1)/BytesPerSec);
        int startByte = dataBase + (currentClus - 2)*SecPerClus*BytesPerSec;


/*      int flag_jump_data;
        flag_jump_data = fseek(fat12,startByte,0);
        if(flag_jump_data == -1)
            printf("fseek in printChildrenFile failed!");

        char* str = (char* )malloc(SecPerClus*BytesPerSec);	//暂存从簇中读出的数据
        char* content = str;

        flag_jump_data = fread(content,1,SecPerClus*BytesPerSec,fat12);
        if(flag_jump_data != SecPerClus*BytesPerSec)
            printf("fread AAAAAAAAAAAAAAin printChildrenFile failed!");
*/
        //现在content相当于一个"根目录区",我们要对其中的条目进行遍历
        int i = 0;
        while(i<SecPerClus*BytesPerSec)
        {


            //建立根目录条目结构体
            struct RootEntry rootEntry;
            struct RootEntry* rootEntry_ptr = &rootEntry;
            int flag_jump_root;
            flag_jump_root = fseek(fat12,startByte,0);
            if(flag_jump_root == -1)
                printf("fseek in printChildrenFile failed!");
            flag_jump_root = fread(rootEntry_ptr,1,32,fat12);
            if(flag_jump_root != 32)
                printf("fread BBBBBBBBBBBBBBBBin printChildrenFile failed!");

            //读取完毕 为下次循环做准备
            i += 32;
            startByte += 32;

            if(rootEntry_ptr->DIR_Name[0] == '\0')
            {   continue;}

            int flag_name_valid = 1;
            int j;
            for (j=0;j<11;j++)
            {
                if (!(((rootEntry_ptr->DIR_Name[j] >= 48)&&(rootEntry_ptr->DIR_Name[j] <= 57)) ||
                      ((rootEntry_ptr->DIR_Name[j] >= 65)&&(rootEntry_ptr->DIR_Name[j] <= 90)) ||
                      ((rootEntry_ptr->DIR_Name[j] >= 97)&&(rootEntry_ptr->DIR_Name[j] <= 122)) ||
                      (rootEntry_ptr->DIR_Name[j] == ' ')))
                {
                    flag_name_valid = 0;	//非英文及数字、空格
                    break;
                }
            }

            if(flag_name_valid == 0)
                continue;

            //名字合法就输出

            if(rootEntry_ptr->DIR_Attr == 0x20)
            {
                //这是一个文件


            }
            else
            {
                char tempName[13];//储存名字
                int realNameIndex = -1;
                int k;
                for(k=0;k<11;++k)//遍历名字中每个字符
                {
                    if(rootEntry_ptr->DIR_Name[k] != ' ')
                    {
                        realNameIndex++;
                        tempName[realNameIndex] = rootEntry_ptr->DIR_Name[k];
                    }
                    else
                    {
                        realNameIndex++;
                        tempName[realNameIndex] = '\0';
                        break;
                    }
                }//目录名已存放到tempName中
                //这是一个目录
                int i=0;
                while(DirName[i] != '\0' && DirName[i] != '/')
                    i++;
                if(DirName[i] == '/')
                    i++;
                strcpy(&DirName[i],tempName);
                DirName[strlen(DirName)] = '/';

                findChildNameInChild(fat12,rootEntry_ptr->DIR_FstClus);
            }




        };


        currentClus = clusValue;//为寻找下个簇做准备

    }

};
void findChildFstClus(FILE* fat12,int startClus)//得到所有子目录的开始簇号 存放在全局变量中
{



    //准备访问数据区对应簇号的簇

    int currentClus = startClus;
    int clusValue = 0;//fat表中对应簇的值

    //循环遍历(目录信息可能存在与多个簇)
    while(clusValue < 0xFF8)
    {
        clusValue = getFATValue(fat12,currentClus);//获得当前值

        if(clusValue == 0xFF7) //坏簇直接退出循环
        {
            printf("坏簇,读取失败!\n");
            break;
        }

        //读取成功,访问数据区对应簇

        int dataBase = BytesPerSec * ( RsvdSecCnt + FATSz*NumFATs + (RootEntCnt*32 + BytesPerSec - 1)/BytesPerSec);
        int startByte = dataBase + (currentClus - 2)*SecPerClus*BytesPerSec;


/*      int flag_jump_data;
        flag_jump_data = fseek(fat12,startByte,0);
        if(flag_jump_data == -1)
            printf("fseek in printChildrenFile failed!");

        char* str = (char* )malloc(SecPerClus*BytesPerSec);	//暂存从簇中读出的数据
        char* content = str;

        flag_jump_data = fread(content,1,SecPerClus*BytesPerSec,fat12);
        if(flag_jump_data != SecPerClus*BytesPerSec)
            printf("fread AAAAAAAAAAAAAAin printChildrenFile failed!");
*/
        //现在content相当于一个"根目录区",我们要对其中的条目进行遍历
        int i = 0;
        while(i<SecPerClus*BytesPerSec)
        {


            //建立根目录条目结构体
            struct RootEntry rootEntry;
            struct RootEntry* rootEntry_ptr = &rootEntry;
            int flag_jump_root;
            flag_jump_root = fseek(fat12,startByte,0);
            if(flag_jump_root == -1)
                printf("fseek in printChildrenFile failed!");
            flag_jump_root = fread(rootEntry_ptr,1,32,fat12);
            if(flag_jump_root != 32)
                printf("fread BBBBBBBBBBBBBBBBin printChildrenFile failed!");

            //读取完毕 为下次循环做准备
            i += 32;
            startByte += 32;

            if(rootEntry_ptr->DIR_Name[0] == '\0')
            {   continue;}

            int flag_name_valid = 1;
            int j;
            for (j=0;j<11;j++)
            {
                if (!(((rootEntry_ptr->DIR_Name[j] >= 48)&&(rootEntry_ptr->DIR_Name[j] <= 57)) ||
                      ((rootEntry_ptr->DIR_Name[j] >= 65)&&(rootEntry_ptr->DIR_Name[j] <= 90)) ||
                      ((rootEntry_ptr->DIR_Name[j] >= 97)&&(rootEntry_ptr->DIR_Name[j] <= 122)) ||
                      (rootEntry_ptr->DIR_Name[j] == ' ')))
                {
                    flag_name_valid = 0;	//非英文及数字、空格
                    break;
                }
            }

            if(flag_name_valid == 0)
                continue;

            //名字合法就输出

            if(rootEntry_ptr->DIR_Attr == 0x20)
            {
                //这是一个文件


            }
            else
            {
                //这是一个目录
                int i=0;
                while(FstClus[i] != 0)
                    i++;

                FstClus[i] = rootEntry_ptr->DIR_FstClus;

                findChildFstClusInChild(fat12,rootEntry_ptr->DIR_FstClus);
            }




        };


        currentClus = clusValue;//为寻找下个簇做准备

    }


};
void findChildFstClusInChild(FILE* fat12,int startClus)
{



    //准备访问数据区对应簇号的簇

    int currentClus = startClus;
    int clusValue = 0;//fat表中对应簇的值

    //循环遍历(目录信息可能存在与多个簇)
    while(clusValue < 0xFF8)
    {
        clusValue = getFATValue(fat12,currentClus);//获得当前值

        if(clusValue == 0xFF7) //坏簇直接退出循环
        {
            printf("坏簇,读取失败!\n");
            break;
        }

        //读取成功,访问数据区对应簇

        int dataBase = BytesPerSec * ( RsvdSecCnt + FATSz*NumFATs + (RootEntCnt*32 + BytesPerSec - 1)/BytesPerSec);
        int startByte = dataBase + (currentClus - 2)*SecPerClus*BytesPerSec;


/*      int flag_jump_data;
        flag_jump_data = fseek(fat12,startByte,0);
        if(flag_jump_data == -1)
            printf("fseek in printChildrenFile failed!");

        char* str = (char* )malloc(SecPerClus*BytesPerSec);	//暂存从簇中读出的数据
        char* content = str;

        flag_jump_data = fread(content,1,SecPerClus*BytesPerSec,fat12);
        if(flag_jump_data != SecPerClus*BytesPerSec)
            printf("fread AAAAAAAAAAAAAAin printChildrenFile failed!");
*/
        //现在content相当于一个"根目录区",我们要对其中的条目进行遍历
        int i = 0;
        while(i<SecPerClus*BytesPerSec)
        {


            //建立根目录条目结构体
            struct RootEntry rootEntry;
            struct RootEntry* rootEntry_ptr = &rootEntry;
            int flag_jump_root;
            flag_jump_root = fseek(fat12,startByte,0);
            if(flag_jump_root == -1)
                printf("fseek in printChildrenFile failed!");
            flag_jump_root = fread(rootEntry_ptr,1,32,fat12);
            if(flag_jump_root != 32)
                printf("fread BBBBBBBBBBBBBBBBin printChildrenFile failed!");

            //读取完毕 为下次循环做准备
            i += 32;
            startByte += 32;

            if(rootEntry_ptr->DIR_Name[0] == '\0')
            {   continue;}

            int flag_name_valid = 1;
            int j;
            for (j=0;j<11;j++)
            {
                if (!(((rootEntry_ptr->DIR_Name[j] >= 48)&&(rootEntry_ptr->DIR_Name[j] <= 57)) ||
                      ((rootEntry_ptr->DIR_Name[j] >= 65)&&(rootEntry_ptr->DIR_Name[j] <= 90)) ||
                      ((rootEntry_ptr->DIR_Name[j] >= 97)&&(rootEntry_ptr->DIR_Name[j] <= 122)) ||
                      (rootEntry_ptr->DIR_Name[j] == ' ')))
                {
                    flag_name_valid = 0;	//非英文及数字、空格
                    break;
                }
            }

            if(flag_name_valid == 0)
                continue;

            //名字合法就输出

            if(rootEntry_ptr->DIR_Attr == 0x20)
            {
                //这是一个文件


            }
            else
            {
                //这是一个目录
                int i=0;
                while(FstClus[i] != 0)
                    i++;

                FstClus[i] = rootEntry_ptr->DIR_FstClus;


                findChildFstClusInChild(fat12,rootEntry_ptr->DIR_FstClus);
            }




        };


        currentClus = clusValue;//为寻找下个簇做准备

    }

};

//返回匹配目录下有多少文件和子目录
void findFileAndDirNum(FILE* fat12,char* yourName,struct RootEntry* rootEntry_ptr)
{

    //刷新全局变量
    FileNum = 0;
    DirNum = 0;
    int x;
    for(x=0;x<1000;++x)
    {
        DirName[x] = '\0';
    };
    for(x=0;x<100;++x)
    {
        FstClus[x] = 0;
    }


    int flag = 0;//标志位:标志是否找到匹配
    //首先为跳转到根目录区做准备：找到偏移量
    int base = (RsvdSecCnt+NumFATs*FATSz)*BytesPerSec;//跨过引导扇区和两个FAT表到达根目录区.
    int flag_jump;//标志位：是否跳转成功或是否读入成功
    char realName[13];//保存将要输出的文件名

    //遍历处理根目录区中的各个条目
    int i;
    for(i=0;i<RootEntCnt;++i)
    {
        //首先跳转
        flag_jump = fseek(fat12,base,0);
        if(flag_jump == -1)
            printf("fseek in printFiles failed!");
        flag_jump = fread(rootEntry_ptr,1,32,fat12);//将根目录区的一条记录读入
        if(flag_jump != 32)
            printf("fread in printFiles failed!");
        //为遍历下一条记录做准备
        base += 32;

        //处理记录
        if(rootEntry_ptr->DIR_Name[0] == '\0')
            continue;//名字为空,空条目不输出

        //若非空 判断名字的有效性 看是否只包含英文数字和空格
        int flag_name_valid = 1;//标志位:判断名字是否有效
        int j;
        for(j=0;j<11;++j)
        {
            //遍历每一个字符
            if (!(((rootEntry_ptr->DIR_Name[j] >= 48)&&(rootEntry_ptr->DIR_Name[j] <= 57)) ||
                  ((rootEntry_ptr->DIR_Name[j] >= 65)&&(rootEntry_ptr->DIR_Name[j] <= 90)) ||
                  ((rootEntry_ptr->DIR_Name[j] >= 97)&&(rootEntry_ptr->DIR_Name[j] <= 122)) ||
                  (rootEntry_ptr->DIR_Name[j] == ' ')))
            {
                flag_name_valid = 0;	//非英文及数字、空格
                break;
            }
        }
        if(!flag_name_valid)
            continue;//若由于名字非法结束遍历,则跳过这个条目

        //若名字合法 判断文件类型 准备输出
        if(rootEntry_ptr->DIR_Attr == 0x20)
        {
            //这是一个文件
            int realNameIndex = -1;
            int k;
            for(k=0;k<11;++k)//遍历名字中每个字符,若遇到一个或多个连续空格,以'.'替代之
            {
                if(rootEntry_ptr->DIR_Name[k] != ' ')
                {
                    realNameIndex++;
                    realName[realNameIndex] = rootEntry_ptr->DIR_Name[k];
                }
                else
                {
                    realNameIndex++;
                    realName[realNameIndex] = '.';
                    while(rootEntry_ptr->DIR_Name[k] == ' ')
                        k++;
                    k--;
                }
            }
            realNameIndex++;
            realName[realNameIndex] = '\0';//文件名已存放到realName中 可以输出

            //比较
            if(strcmp(realName,yourName) == 0) //比较成功 错误!  立刻返回
            {
                printf("输入错误,请输入一个目录!\n");
                flag = 1;
                return;
            }

            //比较不成功 继续循环

        }
        else
        {
            //这是一个目录 同样先把目录名字保存到realName中
            int realNameIndex = -1;
            int k;
            for(k=0;k<11;++k)//遍历名字中每个字符,若遇到一个或多个连续空格,以'.'替代之
            {
                if(rootEntry_ptr->DIR_Name[k] != ' ')
                {
                    realNameIndex++;
                    realName[realNameIndex] = rootEntry_ptr->DIR_Name[k];
                }
                else
                {
                    realNameIndex++;
                    realName[realNameIndex] = '\0';
                    break;
                }
            }
            realNameIndex++;
            realName[realNameIndex] = '\0';//目录名已存放到realName中 可以输出

            if(strcmp(realName,yourName) == 0)//如果匹配 就输出其下面所有文件和目录
            {
                /*	int q=0;
                            while(DirName[q] != '\0' && DirName[q] != '/')
                                q++;
                            if(DirName[q] == '/')
                                q++;
                            int j;
                            for(j=q;j<11+q;++j)
                            {
                                DirName[j] = rootEntry_ptr->DIR_Name[j-q];
                            }
                            DirName[j] = '/';*/


                //输出!
                printf("%s:",realName);
                fileAndDirNumInDir(fat12,rootEntry_ptr->DIR_FstClus);
                printf("%d files,%d directories.\n",FileNum,DirNum);
                int maxDirNum = DirNum;
                findChildName(fat12,rootEntry_ptr->DIR_FstClus);
                findChildFstClus(fat12,rootEntry_ptr->DIR_FstClus);



                     int index = 0;
                     int i;
                     for(i=1;i<=maxDirNum;++i)
                     {
                         //先输出缩进
                         int j;
                         for(j=0;j<i;++j)
                             printf("    ");
                         //输出目录名及信息
                         while(DirName[index] != '/')
                         {
                             printf("%c",DirName[index]);
                             index++;
                         }
                         index++;
                         printf(":");
                         fileAndDirNumInDir(fat12,FstClus[i-1]);
                         printf("%d files,%d directories.\n",FileNum,DirNum);
                     };
                flag = 1;
                return;
            }
            else
            {
                //若不匹配 判断下由于什么原因不匹配
                if(strncmp(realName,yourName,strlen(realName)) == 0)//若头部已经匹配,则继续比较后续
                {
                    flag = findFileAndDirNumInChild(fat12,realName,rootEntry_ptr->DIR_FstClus,yourName);
                    if(flag == 1)
                        return;
                }
                //否则 头部就不匹配 无需比较后续
            }
        }
    }
    if(flag == 0)
        printf("文件/目录 不存在!\n");
};


int findFileAndDirNumInChild(FILE* fat12,char* directory,int startClus,char* yourName)
{
    int flag = 0;
    //将全部路径保存 (若这是个空目录最后再输出)
    char dirName[100] = {0};
    strcpy(dirName,directory);
    dirName[strlen(directory)] = '/';
    char* fatherName = &dirName[strlen(dirName)];

    int flag_null_dir = 0;//标志位:若保持0说明是空目录

    //准备访问数据区对应簇号的簇

    int currentClus = startClus;
    int clusValue = 0;//fat表中对应簇的值

    //循环遍历(目录信息可能存在与多个簇)
    while(clusValue < 0xFF8)
    {
        clusValue = getFATValue(fat12,currentClus);//获得当前值

        if(clusValue == 0xFF7) //坏簇直接退出循环
        {
            printf("坏簇,读取失败!\n");
            break;
        }

        //读取成功,访问数据区对应簇

        int dataBase = BytesPerSec * ( RsvdSecCnt + FATSz*NumFATs + (RootEntCnt*32 + BytesPerSec - 1)/BytesPerSec);
        int startByte = dataBase + (currentClus - 2)*SecPerClus*BytesPerSec;


/*      int flag_jump_data;
        flag_jump_data = fseek(fat12,startByte,0);
        if(flag_jump_data == -1)
            printf("fseek in printChildrenFile failed!");

        char* str = (char* )malloc(SecPerClus*BytesPerSec);	//暂存从簇中读出的数据
        char* content = str;

        flag_jump_data = fread(content,1,SecPerClus*BytesPerSec,fat12);
        if(flag_jump_data != SecPerClus*BytesPerSec)
            printf("fread AAAAAAAAAAAAAAin printChildrenFile failed!");
*/
        //现在content相当于一个"根目录区",我们要对其中的条目进行遍历
        int i = 0;
        while(i<SecPerClus*BytesPerSec)
        {


            //建立根目录条目结构体
            struct RootEntry rootEntry;
            struct RootEntry* rootEntry_ptr = &rootEntry;
            int flag_jump_root;
            flag_jump_root = fseek(fat12,startByte,0);
            if(flag_jump_root == -1)
                printf("fseek in printChildrenFile failed!");
            flag_jump_root = fread(rootEntry_ptr,1,32,fat12);
            if(flag_jump_root != 32)
                printf("fread BBBBBBBBBBBBBBBBin printChildrenFile failed!");

            //读取完毕 为下次循环做准备
            i += 32;
            startByte += 32;

            if(rootEntry_ptr->DIR_Name[0] == '\0')
            {   continue;}

            int flag_name_valid = 1;
            int j;
            for (j=0;j<11;j++)
            {
                if (!(((rootEntry_ptr->DIR_Name[j] >= 48)&&(rootEntry_ptr->DIR_Name[j] <= 57)) ||
                      ((rootEntry_ptr->DIR_Name[j] >= 65)&&(rootEntry_ptr->DIR_Name[j] <= 90)) ||
                      ((rootEntry_ptr->DIR_Name[j] >= 97)&&(rootEntry_ptr->DIR_Name[j] <= 122)) ||
                      (rootEntry_ptr->DIR_Name[j] == ' ')))
                {
                    flag_name_valid = 0;	//非英文及数字、空格
                    break;
                }
            }

            if(flag_name_valid == 0)
                continue;

            //名字合法就输出
            char tempName[13];//储存名字
            if(rootEntry_ptr->DIR_Attr == 0x20)
            {
                //这是一个文件
                int realNameIndex = -1;
                int k;
                for(k=0;k<11;++k)//遍历名字中每个字符,若遇到一个或多个连续空格,以'.'替代之
                {
                    if(rootEntry_ptr->DIR_Name[k] != ' ')
                    {
                        realNameIndex++;
                        tempName[realNameIndex] = rootEntry_ptr->DIR_Name[k];
                    }
                    else
                    {
                        realNameIndex++;
                        tempName[realNameIndex] = '.';
                        while(rootEntry_ptr->DIR_Name[k] == ' ')
                            k++;
                        k--;
                    }
                }
                realNameIndex++;
                tempName[realNameIndex] = '\0';//文件名已存放到tempName中 可以输出

                //比较输出 先输出父目录

                strcpy(fatherName,tempName);
                if(strcmp(dirName,yourName) == 0)
                {
                    printf("输入错误 请输入一个目录!\n");
                    return 0;
                }
            }
            else
            {
                //这是一个目录 同样先把目录名字保存到tempName中
                int realNameIndex = -1;
                int k;
                for(k=0;k<11;++k)//遍历名字中每个字符
                {
                    if(rootEntry_ptr->DIR_Name[k] != ' ')
                    {
                        realNameIndex++;
                        tempName[realNameIndex] = rootEntry_ptr->DIR_Name[k];
                    }
                    else
                    {
                        realNameIndex++;
                        tempName[realNameIndex] = '\0';
                        break;
                    }
                }//目录名已存放到tempName中

                //将子目录名接到父目录名后面
                strcpy(fatherName,tempName);

                if(strcmp(dirName,yourName) == 0)
                {
                    //输出!
                    printf("%s:",dirName);
                    fileAndDirNumInDir(fat12,rootEntry_ptr->DIR_FstClus);
                    printf("%d files,%d directories.\n",FileNum,DirNum);

                    findChildName(fat12,rootEntry_ptr->DIR_FstClus);
                    findChildFstClus(fat12,rootEntry_ptr->DIR_FstClus);


                    int maxDirNum = DirNum;
                    int index = 0;
                    int i;
                    for(i=1;i<=maxDirNum;++i)
                    {
                        //先输出缩进
                        int j;
                        for(j=0;j<i;++j)
                            printf("    ");
                        //输出目录名及信息
                        while(DirName[index] != '/')
                        {
                            printf("%c",DirName[index]);
                            index++;
                        }
                        index++;
                        printf(":");
                        fileAndDirNumInDir(fat12,FstClus[i-1]);
                        printf("%d files,%d directories.\n",FileNum,DirNum);
                    };
                    flag = 1;
                    return 1;
                }
                else
                {
                    if(strncmp(dirName,yourName,strlen(dirName)) == 0)
                    {

                        flag = findFileAndDirNumInChild(fat12,tempName,rootEntry_ptr->DIR_FstClus,yourName);
                        if(flag == 1)
                            return 1;
                    }
                }
            }
        };


        currentClus = clusValue;//为寻找下个簇做准备
    }

    printf("文件/目录 不存在!\n");
    return 0;
};

//返回目录下共有多少文件
void fileAndDirNumInDir(FILE* fat12,int startClus)
{


    FileNum = 0;
    DirNum = 0;
    //准备访问数据区对应簇号的簇

    int currentClus = startClus;
    int clusValue = 0;//fat表中对应簇的值

    //循环遍历(目录信息可能存在与多个簇)
    while(clusValue < 0xFF8)
    {
        clusValue = getFATValue(fat12,currentClus);//获得当前值

        if(clusValue == 0xFF7) //坏簇直接退出循环
        {
            printf("坏簇,读取失败!\n");
            break;
        }

        //读取成功,访问数据区对应簇

        int dataBase = BytesPerSec * ( RsvdSecCnt + FATSz*NumFATs + (RootEntCnt*32 + BytesPerSec - 1)/BytesPerSec);
        int startByte = dataBase + (currentClus - 2)*SecPerClus*BytesPerSec;


/*      int flag_jump_data;
        flag_jump_data = fseek(fat12,startByte,0);
        if(flag_jump_data == -1)
            printf("fseek in printChildrenFile failed!");

        char* str = (char* )malloc(SecPerClus*BytesPerSec);	//暂存从簇中读出的数据
        char* content = str;

        flag_jump_data = fread(content,1,SecPerClus*BytesPerSec,fat12);
        if(flag_jump_data != SecPerClus*BytesPerSec)
            printf("fread AAAAAAAAAAAAAAin printChildrenFile failed!");
*/
        //现在content相当于一个"根目录区",我们要对其中的条目进行遍历
        int i = 0;
        while(i<SecPerClus*BytesPerSec)
        {


            //建立根目录条目结构体
            struct RootEntry rootEntry;
            struct RootEntry* rootEntry_ptr = &rootEntry;
            int flag_jump_root;
            flag_jump_root = fseek(fat12,startByte,0);
            if(flag_jump_root == -1)
                printf("fseek in printChildrenFile failed!");
            flag_jump_root = fread(rootEntry_ptr,1,32,fat12);
            if(flag_jump_root != 32)
                printf("fread BBBBBBBBBBBBBBBBin printChildrenFile failed!");

            //读取完毕 为下次循环做准备
            i += 32;
            startByte += 32;

            if(rootEntry_ptr->DIR_Name[0] == '\0')
            {   continue;}

            int flag_name_valid = 1;
            int j;
            for (j=0;j<11;j++)
            {
                if (!(((rootEntry_ptr->DIR_Name[j] >= 48)&&(rootEntry_ptr->DIR_Name[j] <= 57)) ||
                      ((rootEntry_ptr->DIR_Name[j] >= 65)&&(rootEntry_ptr->DIR_Name[j] <= 90)) ||
                      ((rootEntry_ptr->DIR_Name[j] >= 97)&&(rootEntry_ptr->DIR_Name[j] <= 122)) ||
                      (rootEntry_ptr->DIR_Name[j] == ' ')))
                {
                    flag_name_valid = 0;	//非英文及数字、空格
                    break;
                }
            }

            if(flag_name_valid == 0)
                continue;

            //名字合法就输出

            if(rootEntry_ptr->DIR_Attr == 0x20)
            {
                //这是一个文件
                FileNum++;

            }
            else
            {
                //这是一个目录
                /*  int i=0;
                  while(DirName[i] != '\0' && DirName[i] != '/')
                      i++;
                  if(DirName[i] == '/')
                      i++;
                  int j;
                  for(j=i;j<11+i;++j)
                  {
                      DirName[j] = rootEntry_ptr->DIR_Name[j-i];
                  }
                  DirName[j] = '/';*/
                DirNum++;
                fileAndDirNumInChildDir(fat12,rootEntry_ptr->DIR_FstClus);
            }




        };


        currentClus = clusValue;//为寻找下个簇做准备

    }


};

//子目录下有多少文件和目录
void fileAndDirNumInChildDir(FILE* fat12,int startClus)
{



    //准备访问数据区对应簇号的簇

    int currentClus = startClus;
    int clusValue = 0;//fat表中对应簇的值

    //循环遍历(目录信息可能存在与多个簇)
    while(clusValue < 0xFF8)
    {
        clusValue = getFATValue(fat12,currentClus);//获得当前值

        if(clusValue == 0xFF7) //坏簇直接退出循环
        {
            printf("坏簇,读取失败!\n");
            break;
        }

        //读取成功,访问数据区对应簇

        int dataBase = BytesPerSec * ( RsvdSecCnt + FATSz*NumFATs + (RootEntCnt*32 + BytesPerSec - 1)/BytesPerSec);
        int startByte = dataBase + (currentClus - 2)*SecPerClus*BytesPerSec;


/*      int flag_jump_data;
        flag_jump_data = fseek(fat12,startByte,0);
        if(flag_jump_data == -1)
            printf("fseek in printChildrenFile failed!");

        char* str = (char* )malloc(SecPerClus*BytesPerSec);	//暂存从簇中读出的数据
        char* content = str;

        flag_jump_data = fread(content,1,SecPerClus*BytesPerSec,fat12);
        if(flag_jump_data != SecPerClus*BytesPerSec)
            printf("fread AAAAAAAAAAAAAAin printChildrenFile failed!");
*/
        //现在content相当于一个"根目录区",我们要对其中的条目进行遍历
        int i = 0;
        while(i<SecPerClus*BytesPerSec)
        {


            //建立根目录条目结构体
            struct RootEntry rootEntry;
            struct RootEntry* rootEntry_ptr = &rootEntry;
            int flag_jump_root;
            flag_jump_root = fseek(fat12,startByte,0);
            if(flag_jump_root == -1)
                printf("fseek in printChildrenFile failed!");
            flag_jump_root = fread(rootEntry_ptr,1,32,fat12);
            if(flag_jump_root != 32)
                printf("fread BBBBBBBBBBBBBBBBin printChildrenFile failed!");

            //读取完毕 为下次循环做准备
            i += 32;
            startByte += 32;

            if(rootEntry_ptr->DIR_Name[0] == '\0')
            {   continue;}

            int flag_name_valid = 1;
            int j;
            for (j=0;j<11;j++)
            {
                if (!(((rootEntry_ptr->DIR_Name[j] >= 48)&&(rootEntry_ptr->DIR_Name[j] <= 57)) ||
                      ((rootEntry_ptr->DIR_Name[j] >= 65)&&(rootEntry_ptr->DIR_Name[j] <= 90)) ||
                      ((rootEntry_ptr->DIR_Name[j] >= 97)&&(rootEntry_ptr->DIR_Name[j] <= 122)) ||
                      (rootEntry_ptr->DIR_Name[j] == ' ')))
                {
                    flag_name_valid = 0;	//非英文及数字、空格
                    break;
                }
            }

            if(flag_name_valid == 0)
                continue;

            //名字合法就输出

            if(rootEntry_ptr->DIR_Attr == 0x20)
            {
                //这是一个文件
                FileNum++;

            }
            else
            {
                //这是一个目录
                /* int i=0;
                 while(DirName[i] != '\0' && DirName[i] != '/')
                     i++;
                 if(DirName[i] == '/')
                     i++;
                 int j;
                 for(j=i;j<11+i;++j)
                 {
                     DirName[j] = rootEntry_ptr->DIR_Name[j-i];
                 }
                 DirName[j] = '/';*/
                DirNum++;
                fileAndDirNumInChildDir(fat12,rootEntry_ptr->DIR_FstClus);
            }




        };


        currentClus = clusValue;//为寻找下个簇做准备

    }

};













