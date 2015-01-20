#include <stdlib.h>
#include <stdio.h>

#include "pbs_nodes.h"
#include "id_map.hpp"
#include "u_tree.h"

all_nodes               allnodes;
id_map                  job_mapper;
id_map                  node_mapper;
AvlTree                 ipaddrs = NULL;
bool                    exit_called;
int                     LOGLEVEL = 10;
int                     id;

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

int remove_node(

  all_nodes      *an,
  struct pbsnode *pnode)

  {
  return(0);
  }

void populate_range_string_from_slot_tracker(

  const execution_slot_tracker &est,
  std::string                  &range_str)

  {
  }

id_map::id_map() {}
id_map::~id_map() {}

int id_map::get_new_id(const char *name)

  {
  return(id);
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

const char *id_map::get_name(
    
  int id)

  {
  return(NULL);
  }

struct pbsnode *AVL_find(
    
  u_long   key,
  uint16_t port,
  AvlTree  tree)

  {
  return(NULL);
  }

job *svr_find_job_by_id(

  int jobid)

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

job_usage_info::job_usage_info(int id)
  {
  this->internal_job_id = id;
  }

job_usage_info &job_usage_info::operator =(

  const job_usage_info &other)

  {
  this->internal_job_id = other.internal_job_id;
  this->est = other.est;
  return(*this);
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

void append_link(

  tlist_head *head, /* ptr to head of list */
  list_link  *new_link,  /* ptr to new entry */
  void       *pobj) /* ptr to object to link in */

  {
  }

void *get_next(

  list_link  pl,   /* I */
  char     *file, /* I */
  int      line) /* I */

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

int unlock_ji_mutex(

  job        *pjob,
  const char *id,
  const char *msg,
  int        logging)

  {
  return(0);
  }

void log_err(

  int         errnum,  /* I (errno or PBSErrno) */
  const char *routine, /* I */
  const char *text)    /* I */

  {
  }

int read_val_and_advance(int *val, char **str)
  {
  char *comma;

  if ((*str == NULL) ||
      (val == NULL))
    return(PBSE_BAD_PARAMETER);

  *val = atoi(*str);

  comma = strchr(*str,',');

  if (comma != NULL)
    *str += comma - *str + 1;

  return(PBSE_NONE);
  }

AvlTree AVL_insert( u_long key, uint16_t port, struct pbsnode *node, AvlTree tree )

  {
  return(NULL);
  }

int copy_properties(

  struct pbsnode *dest, /* I */
  struct pbsnode *src)  /* O */

  {
  return(0);
  }
