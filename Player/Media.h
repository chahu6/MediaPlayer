#ifndef MEDIA_H
#define MEDIA_H

//同步阈值下限
#define AV_SYNC_THRESHOLD_MIN 0.04
//同步阈值上限
#define AV_SYNC_THRESHOLD_MAX 0.1
//单帧视频时长阈值上限，用于适配低帧时同步，
//帧率过低视频帧超前不适合翻倍延迟，应特殊
//处理，这里设置上限一秒10帧
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
//同步操作摆烂阈值上限，此时同步已无意义
#define AV_NOSYNC_THRESHOLD 10.0

#define AV_SYNC_REJUDGESHOLD 0.01

#endif // MEDIA_H
