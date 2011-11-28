/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_MEDIAPLAYER_INFO_H
#define ANDROID_MEDIAPLAYER_INFO_H

/* add by Gary. start {{----------------------------------- */
/* 2011-9-13 14:05:05 */
/* expend interfaces about subtitle, track and so on */
#define SUBTITLE_TYPE_TEXT         0
#define SUBTITLE_TYPE_BITMAP       1

#define MEDIAPLAYER_NAME_LEN_MAX                    256

typedef struct _MediaPlayer_SubInfo{
	char    name[MEDIAPLAYER_NAME_LEN_MAX];
	int     len;
    char    charset[MEDIAPLAYER_NAME_LEN_MAX];
	int     type;   // text or bitmap
}MediaPlayer_SubInfo;

typedef struct _MediaPlayer_TrackInfo{
	char    name[MEDIAPLAYER_NAME_LEN_MAX];
	int     len;
    char    charset[MEDIAPLAYER_NAME_LEN_MAX];
}MediaPlayer_TrackInfo;

#define CHARSET_UNKNOWN                        "UNKNOWN"                        //无法识别出来的字符集                  
#define CHARSET_BIG5                           "Big5"                           //繁体中文                        
#define CHARSET_BIG5_HKSCS                     "Big5-HKSCS"                     //                            
#define CHARSET_BOCU_1                         "BOCU-1"                         //                            
#define CHARSET_CESU_8                         "CESU-8"                         //                            
#define CHARSET_CP864                          "cp864"                          //                            
#define CHARSET_EUC_JP                         "EUC-JP"                         //                            
#define CHARSET_EUC_KR                         "EUC-KR"                         //                            
#define CHARSET_GB18030                        "GB18030"                        //                            
#define CHARSET_GBK                            "GBK"                            //简体中文                        
#define CHARSET_HZ_GB_2312                     "HZ-GB-2312"                     //                            
#define CHARSET_ISO_2022_CN                    "ISO-2022-CN"                    //                            
#define CHARSET_ISO_2022_CN_EXT                "ISO-2022-CN-EXT"                //                            
#define CHARSET_ISO_2022_JP                    "ISO-2022-JP"                    //                            
#define CHARSET_ISO_2022_KR                    "ISO-2022-KR"                    //韩文                          
#define CHARSET_ISO_8859_1                     "ISO-8859-1"                     //西欧语系                        
#define CHARSET_ISO_8859_10                    "ISO-8859-10"                    //北欧斯堪的纳维亚语系                  
#define CHARSET_ISO_8859_13                    "ISO-8859-13"                    //波罗的海语系                      
#define CHARSET_ISO_8859_14                    "ISO-8859-14"                    //凯尔特人语系                      
#define CHARSET_ISO_8859_15                    "ISO-8859-15"                    //扩展了法语和芬兰语的西欧语系              
#define CHARSET_ISO_8859_16                    "ISO-8859-16"                    //扩展的东南欧语系                    
#define CHARSET_ISO_8859_2                     "ISO-8859-2"                     //中欧语言                        
#define CHARSET_ISO_8859_3                     "ISO-8859-3"                     //南欧语言                        
#define CHARSET_ISO_8859_4                     "ISO-8859-4"                     //北欧语言                        
#define CHARSET_ISO_8859_5                     "ISO-8859-5"                     //西里尔字母                       
#define CHARSET_ISO_8859_6                     "ISO-8859-6"                     //阿拉伯语                        
#define CHARSET_ISO_8859_7                     "ISO-8859-7"                     //希腊语                         
#define CHARSET_ISO_8859_8                     "ISO-8859-8"                     //希伯来语                        
#define CHARSET_ISO_8859_9                     "ISO-8859-9"                     //土耳其语                        
#define CHARSET_KOI8_R                         "KOI8-R"                         //俄文                          
#define CHARSET_KOI8_U                         "KOI8-U"                         //                            
#define CHARSET_MACINTOSH                      "macintosh"                      //                            
#define CHARSET_SCSU                           "SCSU"                           //                            
#define CHARSET_SHIFT_JIS                      "Shift_JIS"                      //日文                          
#define CHARSET_TIS_620                        "TIS-620"                        //泰文                          
#define CHARSET_US_ASCII                       "US-ASCII"                       //                            
#define CHARSET_UTF_16                         "UTF-16"                         //                            
#define CHARSET_UTF_16BE                       "UTF-16BE"                       //UTF16 big endian            
#define CHARSET_UTF_16LE                       "UTF-16LE"                       //UTF16 little endian         
#define CHARSET_UTF_32                         "UTF-32"                         //                            
#define CHARSET_UTF_32BE                       "UTF-32BE"                       //                            
#define CHARSET_UTF_32LE                       "UTF-32LE"                       //                            
#define CHARSET_UTF_7                          "UTF-7"                          //                            
#define CHARSET_UTF_8                          "UTF-8"                          //UTF8                        
#define CHARSET_WINDOWS_1250                   "windows-1250"                   //中欧                          
#define CHARSET_WINDOWS_1251                   "windows-1251"                   //西里尔文                        
#define CHARSET_WINDOWS_1252                   "windows-1252"                   //土耳其语                        
#define CHARSET_WINDOWS_1253                   "windows-1253"                   //希腊文                         
#define CHARSET_WINDOWS_1254                   "windows-1254"                   //西欧语系                        
#define CHARSET_WINDOWS_1255                   "windows-1255"                   //希伯来文                        
#define CHARSET_WINDOWS_1256                   "windows-1256"                   //阿拉伯文                        
#define CHARSET_WINDOWS_1257                   "windows-1257"                   //波罗的海文                       
#define CHARSET_WINDOWS_1258                   "windows-1258"                   //越南                          
#define CHARSET_X_DOCOMO_SHIFT_JIS_2007        "x-docomo-shift_jis-2007"        //                            
#define CHARSET_X_GSM_03_38_2000               "x-gsm-03.38-2000"               //                            
#define CHARSET_X_IBM_1383_P110_1999           "x-ibm-1383_P110-1999"           //                            
#define CHARSET_X_IMAP_MAILBOX_NAME            "x-IMAP-mailbox-name"            //                            
#define CHARSET_X_ISCII_BE                     "x-iscii-be"                     //                            
#define CHARSET_X_ISCII_DE                     "x-iscii-de"                     //                            
#define CHARSET_X_ISCII_GU                     "x-iscii-gu"                     //                            
#define CHARSET_X_ISCII_KA                     "x-iscii-ka"                     //                            
#define CHARSET_X_ISCII_MA                     "x-iscii-ma"                     //                            
#define CHARSET_X_ISCII_OR                     "x-iscii-or"                     //                            
#define CHARSET_X_ISCII_PA                     "x-iscii-pa"                     //                            
#define CHARSET_X_ISCII_TA                     "x-iscii-ta"                     //                            
#define CHARSET_X_ISCII_TE                     "x-iscii-te"                     //                            
#define CHARSET_X_ISO_8859_11_2001             "x-iso-8859_11-2001"             //                            
#define CHARSET_X_JAVAUNICODE                  "x-JavaUnicode"                  //                            
#define CHARSET_X_KDDI_SHIFT_JIS_2007          "x-kddi-shift_jis-2007"          //                            
#define CHARSET_X_MAC_CYRILLIC                 "x-mac-cyrillic"                 //                            
#define CHARSET_X_SOFTBANK_SHIFT_JIS_2007      "x-softbank-shift_jis-2007"      //                            
#define CHARSET_X_UNICODEBIG                   "x-UnicodeBig"                   //                            
#define CHARSET_X_UTF_16LE_BOM                 "x-UTF-16LE-BOM"                 //                            
#define CHARSET_X_UTF16_OPPOSITEENDIAN         "x-UTF16_OppositeEndian"         //                            
#define CHARSET_X_UTF16_PLATFORMENDIAN         "x-UTF16_PlatformEndian"         //                            
#define CHARSET_X_UTF32_OPPOSITEENDIAN         "x-UTF32_OppositeEndian"         //                            
#define CHARSET_X_UTF32_PLATFORMENDIAN         "x-UTF32_PlatformEndian"         //                            

/*
 * input dimension type list 
 */
#define INPUT_DIMENSION_TYPE_2D                             0   //2D
#define INPUT_DIMENSION_TYPE_3D_FRAME_SEQUENTIAL            1   //·ÖÍ¼¸ñÊ½
#define INPUT_DIMENSION_TYPE_3D_TOP_BOTTOM_HALF             2   //ÉÏÏÂ°ë·ù
#define INPUT_DIMENSION_TYPE_3D_TOP_BOTTOM_FULL             3   //ÉÏÏÂÈ«·ù
#define INPUT_DIMENSION_TYPE_3D_BOTTOM_TOP_HALF             4   //ÏÂÉÏ°ë·ù
#define INPUT_DIMENSION_TYPE_3D_BOTTOM_TOP_FULL             5   //ÏÂÉÏÈ«·ù
#define INPUT_DIMENSION_TYPE_3D_LEFT_RIGHT_HALF             6   //×óÓÒ°ë·ù
#define INPUT_DIMENSION_TYPE_3D_LEFT_RIGHT_FULL             7   //×óÓÒÈ«·ù
#define INPUT_DIMENSION_TYPE_3D_RIGHT_LEFT_HALF             8   //ÓÒ×ó°ë·ù
#define INPUT_DIMENSION_TYPE_3D_RIGHT_LEFT_FULL             9   //ÓÒ×óÈ«·ù
#define INPUT_DIMENSION_TYPE_3D_LINE_INTERLEAVED            10  //ÐÐ½»´í

/*
 * output dimension type list 
 */
#define OUTPUT_DIMENSION_TYPE_DISABLE_3D                    -1  //½ûÖ¹3DÏÔÊ¾
#define OUTPUT_DIMENSION_TYPE_2D_ORGINAL                    0   //2DÏÔÊ¾Ô­Í¼
#define OUTPUT_DIMENSION_TYPE_2D_LEFT_HALF                  1   //2DÏÔÊ¾×ó°ëÍ¼
#define OUTPUT_DIMENSION_TYPE_2D_RIGHT_HALF                 2   //2DÏÔÊ¾ÓÒ°ëÍ¼
#define OUTPUT_DIMENSION_TYPE_2D_TOP_HALF                   3   //2DÏÔÊ¾ÉÏ°ëÍ¼
#define OUTPUT_DIMENSION_TYPE_2D_BOTTOM_HALF                4   //2DÏÔÊ¾ÏÂ°ëÍ¼
#define OUTPUT_DIMENSION_TYPE_3D_LEFT_RIGHT                 5   //3D×óÓÒ£¬½öÓÃÓÚHDMI
#define OUTPUT_DIMENSION_TYPE_3D_TOP_BOTTOM                 6   //3DÉÏÏÂ£¬½öÓÃÓÚHDMI
#define OUTPUT_DIMENSION_TYPE_3D_LINE_INTERLEAVED           7   //3DÐÐ½»´í£¬½öÓÃÓÚHDMI
#define OUTPUT_DIMENSION_TYPE_3D_ANAGLAGH_RED_BLUE          8   //·ÖÉ«ºìÀ¶
#define OUTPUT_DIMENSION_TYPE_3D_ANAGLAGH_RED_GREEN         9   //·ÖÉ«ºìÂÌ
#define OUTPUT_DIMENSION_TYPE_3D_ANAGLAGH_RED_CYAN          10  //·ÖÉ«ºìÇà
#define OUTPUT_DIMENSION_TYPE_3D_ANAGLAGH_FULL_COLOR        11  //·ÖÉ«È«É«
#define OUTPUT_DIMENSION_TYPE_3D_ANAGLAGH_HALF_COLOR        12  //·ÖÉ«°ëÉ«
#define OUTPUT_DIMENSION_TYPE_3D_ANAGLAGH_OPTIMIZED         13  //·ÖÉ«×îÓÅ
#define OUTPUT_DIMENSION_TYPE_3D_ANAGLAGH_YELLOW_BLUE       14  //·ÖÉ«»ÆÀ¶
#define OUTPUT_DIMENSION_TYPE_NAKED_3D_FORMAT_1             15  //½öÓÃÓÚLCD
#define OUTPUT_DIMENSION_TYPE_NAKED_3D_FORMAT_2             16  //½öÓÃÓÚLCD
#define OUTPUT_DIMENSION_TYPE_NAKED_3D_FORMAT_3             17  //½öÓÃÓÚLCD
#define OUTPUT_DIMENSION_TYPE_NAKED_3D_FORMAT_4             18  //½öÓÃÓÚLCD
#define OUTPUT_DIMENSION_TYPE_NAKED_3D_FORMAT_5             19  //½öÓÃÓÚLCD

/*
 * anaglagh type list
 */
#define ANAGLAGH_TYPE_DISABLE                               -1  //²»·ÖÉ«
#define ANAGLAGH_TYPE_RED_BLUE                              0   //·ÖÉ«ºìÀ¶
#define ANAGLAGH_TYPE_RED_GREEN                             1   //·ÖÉ«ºìÂÌ
#define ANAGLAGH_TYPE_RED_CYAN                              2   //·ÖÉ«ºìÇà
#define ANAGLAGH_TYPE_FULL_COLOR                            3   //·ÖÉ«È«É«
#define ANAGLAGH_TYPE_HALF_COLOR                            4   //·ÖÉ«°ëÉ«
#define ANAGLAGH_TYPE_OPTIMIZED                             5   //·ÖÉ«×îÓÅ
#define ANAGLAGH_TYPE_YELLOW_BLUE                           6   //·ÖÉ«»ÆÀ¶

/* add by Gary. end   -----------------------------------}} */

#endif // ANDROID_MEDIAPLAYER_H
