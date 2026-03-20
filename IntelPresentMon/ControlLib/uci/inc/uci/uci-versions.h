#ifndef _UCI_VERSIONS_H_
#define _UCI_VERSIONS_H_

#include <uci/uci-defines.h>

/**************************
 * UCI Collector Versions *
 **************************/

/* SoCWatch */
UCI_DEFINE_GUID(SoCWatchCollectorGUID, 0x54B46B56, 0x5439, 0x4D29, 0x8B07, 0x15863D1F6EC6);
UCI_DEFINE_VERSION(SoCWatchCollectorVersion, 0, 1, 0, 0);
UCI_DEFINE_IDENTIFIER(SoCWatchIdentifier, "socwatch", SoCWatchCollectorGUID, SoCWatchCollectorVersion);

/* Level Zero */
UCI_DEFINE_GUID(LevelZeroCollectorGUID, 0x050BD55C, 0xC420, 0x47B1, 0x8579, 0xC068FD8CD92E);
UCI_DEFINE_VERSION(LevelZeroCollectorVersion, 0, 1, 0, 0);
UCI_DEFINE_IDENTIFIER(LevelZeroIdentifier, "level-zero", LevelZeroCollectorGUID, LevelZeroCollectorVersion);

/* EMON */
UCI_DEFINE_GUID(EMONCollectorGUID, 0xC568FB36, 0x0DE3, 0x4A23, 0xA6F5, 0x2AB298BB9073);
UCI_DEFINE_VERSION(EMONCollectorVersion, 0, 1, 0, 0);
UCI_DEFINE_IDENTIFIER(EMONIdentifier, "emon", EMONCollectorGUID, EMONCollectorVersion);

#if defined(_WIN32)
/* ETW */
UCI_DEFINE_GUID(ETWCollectorGUID, 0x45545720, 0x434F, 0x4C4C, 0x8543, 0x544F52000000);
UCI_DEFINE_VERSION(ETWCollectorVersion, 0, 1, 0, 0);
UCI_DEFINE_IDENTIFIER(ETWIdentifier, "etw", ETWCollectorGUID, ETWCollectorVersion);
#endif

#endif // _UCI_VERSIONS_H_
