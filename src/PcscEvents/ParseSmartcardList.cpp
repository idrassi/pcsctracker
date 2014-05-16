///////////////////////////////////////////////////////////////////////////////
// Name:        ParseSmartcardList.cpp
// Purpose:     convert the file smartcard_list.txt of a C++ header used by pcscTracker
// Author:      Mounir IDRASSI
// Created:     2014-05-14
// Copyright:   (c) 2014 Mounir IDRASSI <mounir.idrassi@idrix.fr>
// Licence:     GPLv3/MIT
///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <string.h>
#include <string>
#include <vector>
#include <tchar.h>

class CEntry
{
public:
   std::string m_szAtr;
   std::vector<std::string> m_szCards;


   CEntry(const CEntry& entry) : m_szAtr(entry.m_szAtr), m_szCards(entry.m_szCards) {}
   CEntry() {}

};

bool isHex(char c)
{
   return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool readLine(char* str, int l, FILE* f)
{
   bool bRet = false;
   size_t len;
   if (fgets(str, l, f))
   {
      len = strlen(str);
      if (len && (str[len - 1] == '\n'))
      {
         str[len - 1] = 0;
      }

      bRet = true;
   }
   return bRet;
}

bool ParseEntry(FILE* g, CEntry& entry)
{
   static char g_szLine[1024];

   bool bRet = false;
   size_t len;

   entry.m_szAtr = "";
   entry.m_szCards.clear();
   
   while(readLine(g_szLine, 1024, g))
   {
      len = strlen(g_szLine);

      if (len && isHex(g_szLine[0]))
      {
         entry.m_szAtr = g_szLine;
         
         while (readLine(g_szLine, 1024, g))
         {
            len = strlen(g_szLine);
            if (len)
            {
               if (g_szLine[0] == '\t')
               {
                  entry.m_szCards.push_back(&g_szLine[1]);
               }
               else
                  break;
            }
            else
               break;
         }

         bRet = true;
         break;
      }
   }

   return bRet;
}

std::string Normalize(const char* str)
{
   static char g_szTxt[1024];
   char c;
   int counter = 0;
   while ((c = *str++))
   {
      if (c == '"' || c == '\\')
      {
         g_szTxt[counter++] = '\\';
      }

      g_szTxt[counter++] = c;
   }

   g_szTxt[counter] = 0;
   return g_szTxt;

}


void WriteEntry(FILE* g, const CEntry& entry)
{
   std::string name;
   

   for (std::vector<std::string>::const_iterator It = entry.m_szCards.begin(); It != entry.m_szCards.end(); It++)
   {
      name += Normalize(It->c_str()) + " | ";      
   }

   if (name.length() >= 3)
   {
      name.erase(name.length() - 3, 3);
   }

   fprintf(g, "\t{ \"%s\", \"%s\"},\n", entry.m_szAtr.c_str(), name.c_str());
}


int _tmain(int argc, _TCHAR* argv[])
{
   if (argc != 2)
   {
      printf("Convert smartcard_list.txt from pcsclite to smartcard_list.h used by PcscEvents program.\n");
      printf("Usage : %s smartcard_list.txt\n\n", argv[0]);
      return -1;
   }

   FILE* f = _tfopen(argv[1], _T("rt"));
   if (f)
   {
      FILE* g = _tfopen(_T("smartcard_list.h"), _T("wt"));
      if (g)
      {
         std::vector<CEntry> list;
         CEntry entry;
         while (ParseEntry(f, entry))
         {
            list.push_back(entry);
         }

         fprintf(g,"#pragma once\n\n");
         fprintf(g,"typedef struct{\n\tconst char* atr;\n\tconst char* cardName;\n}\nCardEntry;\n\n");
         fprintf(g,"static CardEntry g_cardsList[] = { \n");

         for (std::vector<CEntry>::iterator It = list.begin(); It != list.end(); It++)
         {
            WriteEntry(g, *It);
         }

         fprintf(g,"};\n\n");

         fclose(g);
      }
      else
      {
         printf("Failed to open file smartcard_list.h for writing.\n");
      }

      fclose(f);
   }
   else
   {
      printf("Failed to open file \"%s\" for reading\n", argv[1]);
   }


	return 0;
}

