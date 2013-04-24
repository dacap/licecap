/*
    WDL - lineparse.h
    Copyright (C) 2005 Cockos Incorporated
    Copyright (C) 1999-2004 Nullsoft, Inc. 
  
    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
      
*/

/*

  This file provides a simple line parsing class. This class is also used in NSIS,
  http://nsis.sf.net. In particular, it allows for multiple space delimited tokens 
  on a line, with a choice of three quotes (`bla`, 'bla', or "bla") to contain any 
  items that may have spaces.

  For a bigger reference on the format, you can refer to NSIS's documentation.
  
*/

#ifndef WDL_LINEPARSE_H_
#define WDL_LINEPARSE_H_


#ifndef WDL_LINEPARSE_IMPL_ONLY
class LineParser 
{
  public:
    int getnumtokens() const { return (m_nt-m_eat)>=0 ? (m_nt-m_eat) : 0; }
    #ifdef WDL_LINEPARSE_INTF_ONLY
      int parse(const char *line, int ignore_escaping=1); //-1 on error

      double gettoken_float(int token, int *success=NULL) const;
      int gettoken_int(int token, int *success=NULL) const;
      unsigned int gettoken_uint(int token, int *success=NULL) const;
      const char *gettoken_str(int token) const;
      int gettoken_enum(int token, const char *strlist) const; // null seperated list

      void set_one_token(const char *ptr);
    #endif

    void eattoken() { m_eat++; }
    bool InCommentBlock() const { return m_bCommentBlock; }


    LineParser(bool bCommentBlock=false)
    {
      m_bCommentBlock=bCommentBlock;
      m_nt=m_eat=0;
      m_tokens=0;
      m_tmpbuf_used=0;
    }
    
    ~LineParser()
    {
      freetokens();
    }

#endif // !WDL_LINEPARSE_IMPL_ONLY


    
#ifndef WDL_LINEPARSE_INTF_ONLY
   #ifdef WDL_LINEPARSE_IMPL_ONLY
     #define WDL_LINEPARSE_PREFIX LineParser::
     #define WDL_LINEPARSE_DEFPARM(x)
   #else
     #define WDL_LINEPARSE_PREFIX
     #define WDL_LINEPARSE_DEFPARM(x) =(x)
   #endif

    int WDL_LINEPARSE_PREFIX parse(const char *line, int ignore_escaping WDL_LINEPARSE_DEFPARM(1))
    {
      freetokens();
      bool bPrevCB=m_bCommentBlock;
      int n=doline(line, ignore_escaping);
      if (n) { m_nt=0; return n; }
      if (m_nt) 
      {
        m_bCommentBlock=bPrevCB;
        m_tokens=(char**)tmpbufalloc(sizeof(char*)*m_nt);
        if (m_tokens) memset(m_tokens,0,m_nt * sizeof(char*));
        n=doline(line, ignore_escaping);
        if (n) 
        {
          freetokens();
          return -1;
        }
      }
      return 0;
    }
    void WDL_LINEPARSE_PREFIX set_one_token(const char *ptr) 
    { 
      freetokens();
      m_eat=0;
      m_nt=1;
      m_tokens=(char **)tmpbufalloc(sizeof(char *));
      m_tokens[0]=tmpbufalloc(strlen(ptr)+1);
      if (m_tokens[0]) strcpy(m_tokens[0],ptr);
    }

    double WDL_LINEPARSE_PREFIX gettoken_float(int token, int *success WDL_LINEPARSE_DEFPARM(NULL)) const
    {
      token+=m_eat;
      if (token < 0 || token >= m_nt || !m_tokens || !m_tokens[token]) 
      {
        if (success) *success=0;
        return 0.0;
      }
      char *t=m_tokens[token];
      if (success)
        *success=*t?1:0;

      char buf[512];
      char *ot=buf;
      while (*t&&(ot-buf)<500) 
      {
        char c=*t++;
        if (success && (c < '0' || c > '9')&&c != '.'&&c!=',') *success=0;
        *ot++=c==','?'.':c;
      }
      *ot=0;
      return atof(buf);
    }

    int WDL_LINEPARSE_PREFIX gettoken_int(int token, int *success WDL_LINEPARSE_DEFPARM(NULL)) const
    { 
      token+=m_eat;
      if (token < 0 || token >= m_nt || !m_tokens || !m_tokens[token] || !m_tokens[token][0]) 
      {
        if (success) *success=0;
        return 0;
      }
      char *tmp;
      int l;
      if (m_tokens[token][0] == '-') l=strtol(m_tokens[token],&tmp,0);
      else l=(int)strtoul(m_tokens[token],&tmp,0);
      if (success) *success=! (int)(*tmp);
      return l;
    }

    unsigned int WDL_LINEPARSE_PREFIX gettoken_uint(int token, int *success WDL_LINEPARSE_DEFPARM(NULL)) const
    { 
      token+=m_eat;
      if (token < 0 || token >= m_nt || !m_tokens || !m_tokens[token] || !m_tokens[token][0]) 
      {
        if (success) *success=0;
        return 0;
      }
      char *tmp;      
      const char* p=m_tokens[token];
      if (p[0] == '-') ++p;
      unsigned int val=(int)strtoul(p, &tmp, 0);
      if (success) *success=! (int)(*tmp);
      return val;
    }

    const char * WDL_LINEPARSE_PREFIX gettoken_str(int token) const
    { 
      token+=m_eat;
      if (token < 0 || token >= m_nt || !m_tokens || !m_tokens[token]) return "";
      return m_tokens[token]; 
    }

    int WDL_LINEPARSE_PREFIX gettoken_enum(int token, const char *strlist) const // null seperated list
    {
      int x=0;
      const char *tt=gettoken_str(token);
      if (tt && *tt) while (*strlist)
      {
#ifdef _WIN32
        if (!stricmp(tt,strlist)) return x;
#else
        if (!strcasecmp(tt,strlist)) return x;
#endif
        strlist+=strlen(strlist)+1;
        x++;
      }
      return -1;
    }

#ifndef WDL_LINEPARSE_IMPL_ONLY
  private:
#endif

    void WDL_LINEPARSE_PREFIX freetokens()
    {
      if (m_tokens)
      {
        int x;
        for (x = 0; x < m_nt; x ++) tmpbuffree(m_tokens[x]);
        tmpbuffree((char*)m_tokens);
      }
      m_tmpbuf_used=0;
      m_tokens=0;
      m_nt=0;
    }

    int WDL_LINEPARSE_PREFIX doline(const char *line, int ignore_escaping)
    {
      m_nt=0;
      if ( m_bCommentBlock )
      {
        while ( *line )
        {
          if ( *line == '*' && *(line+1) == '/' )
          {
            m_bCommentBlock=false; // Found end of comment block
            line+=2;
            break;
          }
          line++;
        }
      }
      while (*line == ' ' || *line == '\t') line++;
      while (*line) 
      {
        int lstate=0; // 1=", 2=`, 4='
        if (*line == ';' || *line == '#') break;
        if (*line == '/' && *(line+1) == '*')
        {
          m_bCommentBlock = true;
          line+=2;
          return doline(line, ignore_escaping);
        }
        if (*line == '\"') lstate=1;
        else if (*line == '\'') lstate=2;
        else if (*line == '`') lstate=4;
        if (lstate) line++;
        int nc=0;
        const char *p = line;
        while (*line)
        {
          if (line[0] == '$' && line[1] == '\\') {
            switch (line[2]) {
              case '"':
              case '\'':
              case '`':
                nc += ignore_escaping ? 3 : 1;
                line += 3;
                continue;
            }
          }
          if (lstate==1 && *line =='\"') break;
          if (lstate==2 && *line =='\'') break;
          if (lstate==4 && *line =='`') break;
          if (!lstate && (*line == ' ' || *line == '\t')) break;
          line++;
          nc++;
        }
        if (m_tokens)
        {
          int i;
          m_tokens[m_nt]=tmpbufalloc(nc+1);
          for (i = 0; p < line; i++, p++) {
            if (!ignore_escaping && p[0] == '$' && p[1] == '\\') {
              switch (p[2]) {
                case '"':
                case '\'':
                case '`':
                  p += 2;
              }
            }
            m_tokens[m_nt][i] = *p;
          }
          m_tokens[m_nt][nc]=0;
        }
        m_nt++;
        if (lstate)
        {
          if (*line) line++;
          else return -2;
        }
        while (*line == ' ' || *line == '\t') line++;
      }
      return 0;
    }

    char * WDL_LINEPARSE_PREFIX tmpbufalloc(int sz)
    {
      if (sz<1)sz=1;
      if (sz+m_tmpbuf_used <= (int)sizeof(m_tmpbuf))
      {
         m_tmpbuf_used+=sz;
         return m_tmpbuf + m_tmpbuf_used - sz;
      }
      return (char *)malloc(sz);
    }
    void WDL_LINEPARSE_PREFIX tmpbuffree(char *p)
    {
      if (p < m_tmpbuf || p >= m_tmpbuf + sizeof(m_tmpbuf)) free(p);
    }

   #undef WDL_LINEPARSE_PREFIX
   #undef WDL_LINEPARSE_DEFPARM
#endif // ! WDL_LINEPARSE_INTF_ONLY
    
#ifndef WDL_LINEPARSE_IMPL_ONLY
  private:

#ifdef WDL_LINEPARSE_INTF_ONLY
    void freetokens();
    int doline(const char *line, int ignore_escaping);
    char *tmpbufalloc(int sz);
    void tmpbuffree(char *p);
#endif

    int m_eat;
    int m_nt;
    int m_tmpbuf_used;
    bool m_bCommentBlock;
    char **m_tokens;
    char m_tmpbuf[2048];
};
#endif//!WDL_LINEPARSE_IMPL_ONLY
#endif//WDL_LINEPARSE_H_

