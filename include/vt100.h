/* 
 * File:   vt100.h
 * Author: wyy
 *
 * Created on 2015年7月23日, 上午10:54
 */

#ifndef VT100_H
#define	VT100_H

#ifdef	__cplusplus
extern "C" {
#endif


//===============================================ANSI控制码的说明
#define VT100_FLAG0         "\x1b"          //ESC
#define VT100_FLAG1         "["
#define VT100_FLAG          VT100_FLAG0 VT100_FLAG1
#define VT100_HEAD          VT100_FLAG
#define VT100_UP(n)         VT100_HEAD n "A"    //ESC[nA 光标上移n行
#define VT100_DOWN(n)       VT100_HEAD n "B"    //ESC[nB 光标下移n行
#define VT100_RIGHT(n)      VT100_HEAD n "C"    //ESC[nC 光标右移n行
#define VT100_LEFT(n)       VT100_HEAD n "D"    //ESC[nD 光标左移n行
#define VT100_CLR2EOF       VT100_HEAD "J"      //
#define VT100_CLR_EOL       VT100_HEAD "K"      //ESC[K 清除从光标到行尾的内容
#define VT100_CLEAR         VT100_HEAD "2J"     //ESC[2J 清屏
#define VT100_SAVE_CURSOR   VT100_HEAD "s"      //ESC[s 保存光标位置
#define VT100_LOAD_CURSOR   VT100_HEAD "u"      //ESC[u 恢复光标位置
#define VT100_HIDE_CURSOR   VT100_HEAD "?25l"   //ESC[?25l 隐藏光标
#define VT100_SHOW_CURSOR   VT100_HEAD "?25h"   //ESC[?25h 显示光标
#define VT100_RESET         VT100_HEAD "0m"     //ESC[0m 关闭所有属性
#define VT100_HIGHT_LIGHT   VT100_HEAD "1m"     //ESC[1m 设置高亮度
#define VT100_UNDER_LINE    VT100_HEAD "4m"     //ESC[4m 下划线
#define VT100_FLASH         VT100_HEAD "5m"     //ESC[5m 闪烁
#define VT100_INVERT        VT100_HEAD "7m"     //ESC[7m 反显
#define VT100_INVISIBLE     VT100_HEAD "8m"     //ESC[8m 消隐
#define VT100_GOTO(x,y)     VT100_HEAD x ";" y "H" VT100_CLR_EOL    //ESC[y;xH设置光标位置
#define VT100_COLOR(b,f)    VT100_HEAD b ";" f "m"                  //
//ESC[30m&nbsp-- ESC[37m 设置前景色
//ESC[40m&nbsp-- ESC[47m 设置背景色

//字颜色:30-----------39
#define VT100_F_BLACK       "30"    //黑
#define VT100_F_RED         "31"    //红
#define VT100_F_GREEN       "32"    //绿
#define VT100_F_YELLOW      "33"    //黄
#define VT100_F_BLUE        "34"    //蓝色
#define VT100_F_PINK        "35"    //紫色
#define VT100_F_DARK_G      "36"    //深绿
#define VT100_F_WHITE       "37"    //白色

//字背景颜色范围:40----49
#define VT100_B_BLACK       "40"    //黑
#define VT100_B_RED         "41"    //深红
#define VT100_B_GREEN       "42"    //绿
#define VT100_B_YELLOW      "43"    //黄色
#define VT100_B_BLUE        "44"    //蓝色
#define VT100_B_PINK        "45"    //紫色
#define VT100_B_DARK_G      "46"    //深绿
#define VT100_B_WHITE       "47"    //白色

//#define VT100_STYLE0        VT100_RESET
#define VT100_STYLE_BAR_A0   (VT100_INVERT VT100_COLOR(VT100_B_BLACK,VT100_F_GREEN))
#define VT100_STYLE_BAR_A1   (VT100_INVERT VT100_COLOR(VT100_B_BLACK,VT100_F_RED))
#define VT100_STYLE_BAR_B0   (VT100_INVERT VT100_COLOR(VT100_B_BLACK,VT100_F_GREEN))
#define VT100_STYLE_BAR_B1   (VT100_INVERT VT100_COLOR(VT100_B_BLACK,VT100_F_RED))
#define VT100_STYLE_VALUE   VT100_COLOR(VT100_B_BLACK,VT100_F_YELLOW) VT100_HIGHT_LIGHT
#define VT100_STYLE_LED_A(r)  (r ? VT100_STYLE_BAR_A1 : VT100_STYLE_BAR_A0)
#define VT100_STYLE_LED_B(r)  (r ? VT100_STYLE_BAR_B1 : VT100_STYLE_BAR_B0)
#define VT100_STYLE0        VT100_COLOR(VT100_B_BLACK,VT100_F_GREEN)
#define VT100_STYLE1        VT100_COLOR(VT100_B_BLACK,VT100_F_WHITE) VT100_HIGHT_LIGHT
#define VT100_STYLE2        VT100_COLOR(VT100_B_DARK_G,VT100_F_WHITE)
#define VT100_STYLE_BLUE    VT100_INVERT VT100_COLOR(VT100_B_BLUE,VT100_F_WHITE)
#define VT100_STYLE_DARK    VT100_INVERT VT100_COLOR(VT100_B_BLACK,VT100_F_YELLOW)
#define VT100_STYLE_TITLE   VT100_COLOR(VT100_B_BLACK,VT100_F_GREEN) VT100_HIGHT_LIGHT
#define VT100_STYLE_LIGHT   VT100_COLOR(VT100_B_BLACK,VT100_F_YELLOW) VT100_HIGHT_LIGHT VT100_UNDER_LINE VT100_FLASH
#define VT100_STYLE_ALERT   VT100_COLOR(VT100_B_RED,VT100_F_YELLOW) VT100_HIGHT_LIGHT
#define VT100_STYLE_NORMAL  VT100_COLOR(VT100_B_YELLOW,VT100_F_WHITE)
#define VT100_STYLE_MENU    VT100_RESET VT100_COLOR(VT100_B_BLACK,VT100_F_DARK_G)
#define VT100_STYLE_KEY     VT100_COLOR(VT100_B_BLACK,VT100_F_YELLOW) VT100_HIGHT_LIGHT VT100_UNDER_LINE
#define VT100_STYLE_HOT     VT100_RESET VT100_COLOR(VT100_B_BLACK,VT100_F_RED) VT100_HIGHT_LIGHT

char*   VT100_GOTO_XY(int x,int y);


#ifdef	__cplusplus
}
#endif

#endif	/* VT100_H */

