#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include "pbs_nodes.h"
#include "id_map.hpp"
#include "u_tree.h"

int LOGLEVEL = 5;
all_nodes               allnodes;
id_map          job_mapper;
id_map                node_mapper;
AvlTree                 ipaddrs = NULL;
bool exit_called;


int lock_node(struct pbsnode *pnode, const char *caller, const char *msg, int level) {return(0);}
int unlock_node(struct pbsnode *pnode, const char *caller, const char *msg, int level) {return(0);}

int hasprop(

  struct pbsnode *pnode,
  struct prop    *props)

  {
  struct  prop    *need;

  for (need = props;need;need = need->next)
    {

    struct prop *pp;

    if (need->mark == 0) /* not marked, skip */
      continue;

    for (pp = pnode->nd_first;pp != NULL;pp = pp->next)
      {
      if (strcmp(pp->name, need->name) == 0)
        break;  /* found it */
      }

    if (pp == NULL)
      {
      return(0);
      }
    }

  return(1);
  }  /* END hasprop() */


int number(

  char **ptr,
  int   *num)

  {
  char  holder[80];
  int   i = 0;
  char *str = *ptr;

  while (isdigit(*str) && (unsigned int)(i + 1) < sizeof holder)
    holder[i++] = *str++;

  if (i == 0)
    {
    return(1);
    }

  holder[i] = '\0';

  if ((i = atoi(holder)) <= 0)
    {
    return(-1);
    }

  *ptr = str;

  *num = i;

  return(0);
  }  /* END number() */



int property(

  char **ptr,
  char **prop)

  {
  char        *str = *ptr;
  char        *dest = *prop;
  int          i = 0;

  while (isalnum(*str) || *str == '-' || *str == '.' || *str == '=' || *str == '_')
    dest[i++] = *str++;

  dest[i] = '\0';

  /* skip over "/vp_number" */

  if (*str == '/')
    {
    do
      {
      str++;
      }
    while (isdigit(*str));
    }

  *ptr = str;

  return(0);
  }  /* END property() */

int proplist(char **str, struct prop **plist, int *node_req, int *gpu_req)
  {
  struct prop *pp;
  char         name_storage[80];
  char        *pname;
  char        *pequal;

  *node_req = 1; /* default to 1 processor per node */

  pname  = name_storage;
  *pname = '\0';

  for (;;)
    {
    if (property(str, &pname))
      {
      return(1);
      }

    if (*pname == '\0')
      break;

    if ((pequal = strchr(pname, (int)'=')) != NULL)
      {
      /* special property */

      /* identify the special property and place its value */
      /* into node_req       */

      *pequal = '\0';

      if (strcmp(pname, "ppn") == 0)
        {
        pequal++;

        if ((number(&pequal, node_req) != 0) || (*pequal != '\0'))
          {
          return(1);
          }
        }
      else if (strcmp(pname, "gpus") == 0)
        {
        pequal++;

        if ((number(&pequal, gpu_req) != 0) || (*pequal != '\0'))
          {
          return(1);
          }
        }
      else
        {
        return(1); /* not recognized - error */
        }
      }
    else
      {
      pp = (struct prop *)calloc(1, sizeof(struct prop));

      pp->mark = 1;
      pp->name = strdup(pname);
      pp->next = *plist;

      *plist = pp;
      }

    if (**str != ':')
      break;

    (*str)++;
    }  /* END for(;;) */

  return(PBSE_NONE);
  } /* END proplist() */



void log_ext(

  int         errnum,   /* I (errno or PBSErrno) */
  const char *routine,  /* I */
  const char *text,     /* I */
  int         severity) /* I */

  {
  }

int pbs_getaddrinfo(
    
  const char       *pNode,
  struct addrinfo  *pHints,
  struct addrinfo **ppAddrInfoOut)

  {
  return(0);
  }

int decode_arst(

  pbs_attribute *patr,    /* O (modified) */
  const char   *name,    /* I pbs_attribute name (notused) */
  const char *rescn,   /* I resource name (notused) */
  const char    *val,     /* I pbs_attribute value */
  int            perm) /* only used for resources */

  {
  return(0);
  }

int find_attr(

  struct attribute_def *attr_def, /* ptr to pbs_attribute definitions */
  const char           *name,     /* pbs_attribute name to find */
  int                   limit)    /* limit on size of def array */

  {
  return(0);
  }

int remove_node(

  all_nodes      *an,
  struct pbsnode *pnode)

  {
  return(0);
  }

job_usage_info::job_usage_info(int count)
  {
  }

job_usage_info &job_usage_info::operator =(

  const job_usage_info &other)

  {
  return(*this);
  }

void log_event(

  int         eventtype,
  int         objclass,
  const char *objname,
  const char *text)

  {
  }

void log_record(

  int         eventtype,  /* I */
  int         objclass,   /* I */
  const char *objname,    /* I */
  const char *text)       /* I */

  {
  }

void populate_range_string_from_slot_tracker(

  const execution_slot_tracker &est,
  std::string                  &range_str)

  {
  }

void append_link(

  tlist_head *head, /* ptr to head of list */
  list_link  *new_link,  /* ptr to new entry */
  void       *pobj) /* ptr to object to link in */

  {
  }

id_map::id_map() {}
id_map::~id_map() {}

const char *id_map::get_name(int id)
  {
  return(NULL);
  }

int id_map::get_new_id(const char *name)

  {
  return(0);
  }

void *get_next(

  list_link  pl,   /* I */
  char     *file, /* I */
  int      line) /* I */

  {
  return(NULL);
  }

struct prop *init_prop(

  const char *pname) /* I */

  {
  return(NULL);
  }

int node_gpustatus_list(

  pbs_attribute *new_attr,      /* derive status into this pbs_attribute*/
  void          *pnode,    /* pointer to a pbsnode struct     */
  int            actmode)  /* action mode; "NEW" or "ALTER"   */

  {
  return(0);
  }

void free_prop_list(
    
  struct prop *prop)

  {
  }

struct pbsnode *AVL_find(
    
  u_long   key,
  uint16_t port,
  AvlTree  tree)

  {
  return(NULL);
  }

svrattrl *attrlist_create(

  const char  *aname, /* I - pbs_attribute name */
  const char  *rname, /* I - resource name if needed or null */
  int           vsize) /* I - size of resource value         */

  {
  return(NULL);
  }

AvlTree AVL_delete_node( 

  u_long   key, 
  uint16_t port, 
  AvlTree  tree)

  {
  return(NULL);
  }

int unlock_ji_mutex(

  job        *pjob,
  const char *id,
  const char *msg,
  int        logging)

  {
  return(0);
  }

job *svr_find_job_by_id(int jobid)
  {
  return(NULL);
  }

void log_err(

  int         errnum,  /* I (errno or PBSErrno) */
  const char *routine, /* I */
  const char *text)    /* I */

  {
  }

AvlTree AVL_insert( u_long key, uint16_t port, struct pbsnode *node, AvlTree tree )

  {
  return(NULL);
  }

int read_val_and_advance(

  int   *val,
  char **str)

  {
  return(0);
  }

int copy_properties(

  struct pbsnode *dest, /* I */
  struct pbsnode *src)  /* O */

  {
  return(0);
  }
