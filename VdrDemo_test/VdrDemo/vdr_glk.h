#ifndef __VDR_GLK_H
#define __VDR_GLK_H

#include <stdint.h>

// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 DOWNLOAD_VDR_DLL_APIS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// DOWNLOAD_DLL_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef DOWNLOAD_DLL_EXPORTS
#define VDR_DLL_API __declspec(dllexport)
#else
#define VDR_DLL_API __declspec(dllimport)
#endif


#ifdef __cplusplus
extern "C" {
#endif

#ifndef __VDR_DATA_TYPE
#define __VDR_DATA_TYPE
	enum datatype_t
	{
		DT_CAN = 1,
		DT_AUDIO = 2,
		DT_LOG = 4,
		DT_CONFIG = 8,
		DT_RADAR = 16,
		DT_VIDEO = 32,
		DT_SONAR = 64,
		DT_ETHERNET = 128,
	};
#endif


/**
 * @brief vdrdown_idc 下载白匣子的数据，采用非阻塞模式
 * @param path 下载数据本地保存绝对路径
 * @param begintime 开始下载时间字符串，精确到分，缺省则从最初时间下载
 * @param endtime 结束下载时间字符串，精确到分，缺省则下载至最新数据
 * @param downtype 下载的数据种类，可以通过“或运算”表示同时下载多种数据
 * @return 0：成功；-1：失败
 */
VDR_DLL_API int vdrdown_idc(char* path, char* begintime, char* endtime, long downtype);


/**
 * @brief vdrdown_pdc 下载黑匣子的数据，采用非阻塞模式
 * @param path 下载数据本地保存绝对路径
 * @param datasize 需要下载的容量，以MB为单位，容量为所要下载数据的总容量，函数内部根据设置待下载类型数据量的比值，自动分配各类数据的下载容量，达到所下载各类数据的起始时间接近
 * @param downtype 下载的数据种类，可以通过“或运算”表示同时下载多种数据
 * @return 0：成功；-1：失败
 */
VDR_DLL_API int  vdrdown_pdc(char* path, int  datasize, long downtype);

/**
 * @brief OnProgress 查询当前下载状态，检测仪程序1秒钟调用一次，更新进度条
 * @param filepath 返回当前正在下载文件在下载存储目录下的相对路径文件名
 * @param size filepath字符串缓冲区字节数，返回字符串长度不能超过此值
 * @param percent 返回已下载的百分比（100表示已全部下载完成）
 */
VDR_DLL_API void  onprogress(char* filepath, int size, int *percent);


/**
 * @brief  queryDiskSyncStatus 查询下载数据的硬盘同步状态(下载数据缓冲区解析存储操作)
 * @param  isIdc  =1查询内存下载数据同步状态，=0查询查询外存下载数据同步状态
 * @param  0:同步未开始/同步完成(缓冲区数据处理完毕），1:正在同步中
 */
VDR_DLL_API int  queryDiskSyncStatus(int isIdc);

/**
 * @brief getspeed 查询当前的下载速度
 * @return  下载速度整数化表示  speed*1000
 */
VDR_DLL_API int getspeed();

/**
 * @brief vdrstop 设置中断停止下载
 * @return 0：成功；-1：失败
 */
VDR_DLL_API int  vdrstop();

/**
 * @brief set_version 设置调用DLL接口函数的厂商发布版本号
 * @param ver 接口函数的厂商发布版本号
 * @return 0：设置成功；-1：设置失败，当前DLL文件中不存在该接口函数版本
 */
VDR_DLL_API int  set_version(int ver);

/**
 * @brief vdrdown_query 查询主机目录分布表
 * @param mode 整形指针，返回主机的目录存储模式，当前校时新建目录模式对应值为“1”
 * @param num 整形指针，返回主机上的校时目录个数
 * @param map 录分布表缓冲区指针，为了不使用结构体（DLL接口使用结构体可能造成不安全），存储规则见表5
 * @param mapsize map缓冲区的字节数
 * @return 0成功，-1失败
 */
VDR_DLL_API int  vdrdown_query(int *mode, int *num, char *map, int mapsize);

/**
 * @brief vdrdown_set_dir  设置内存储器数据的当前下载目录，当没有调用该函数时或调用该函数失败时，以当前存储的校时目录为下载目录
 * @param dir 待下载的内存储器目录
 * @return 0成功，-1失败
 */
VDR_DLL_API int  vdrdown_set_dir(char *dir);

#ifdef __cplusplus
}
#endif

#endif