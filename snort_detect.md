* ## [snort匹配引擎概述](#1-snort匹配引擎概述) 
* ## [AC算法](#2-AC算法)

### snort匹配引擎概述
snort中负责管理多模式匹配的源文件是mpse.h(multiple pattern search engine),对用于匹配的算法进行选择。

在mpse_methods中定义了主要的算法，其中主要分为两大类:AC算法和bnfa算法，基于的原理有dfa和nfa两种
```c
/* Pattern Matching Methods*/
//#define MPSE_MWM       1
#define MPSE_AC        2
//#define MPSE_KTBM      3
#define MPSE_LOWMEM    4
//#define MPSE_AUTO      5
#define MPSE_ACF       6
#define MPSE_ACS       7
#define MPSE_ACB       8
#define MPSE_ACSB      9
#define MPSE_AC_BNFA   10  // 基于NFA状态机的AC算法
#define MPSE_AC_BNFA_Q 11  // 基于NFA状态机的AC算法
#define MPSE_ACF_Q     12  //利用Trie树
#define MPSE_LOWMEM_Q  13 //硬件加速  http://www.voidcn.com/article/p-rguwsyeb-bdg.html
```











### AC算法

AC ( Aho-Corasick)算法
