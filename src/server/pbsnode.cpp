#include <stdlib.h>
#include <stdio.h>

#include "pbs_nodes.h"
#include "net_cache.h"
#include "id_map.hpp"
#include "../lib/Libnet/lib_net.h"
#include "log.h"
#include "timer.hpp"
#include "pbs_job.h"
#include "u_tree.h"

void populate_range_string_from_slot_tracker(const execution_slot_tracker &est, std::string &range_str);
int  unlock_ji_mutex(job *pjob, const char *id, const char *msg, int logging);
void free_prop_list(struct prop *prop);
int read_val_and_advance(int *val, char **str);

extern AvlTree ipaddrs;



pbsnode::pbsnode(

  const char *pname, /* node name */
  u_long     *pul,  /* host byte order array of ipaddrs for this node */
  bool        skip_lookup) : nd_first(NULL), nd_last(NULL), nd_f_st(NULL), nd_l_st(NULL),
                             nd_addrs(NULL), nd_prop(NULL), nd_status(NULL), nd_note(NULL),
                             nd_flag(okay), nd_mom_port(0), nd_mom_rm_port(0),
                             nd_sock_addr(), nd_nprops(0), nd_slots(), nd_job_usages(),
                             nd_needed(0), nd_np_to_be_used(0), nd_state(0), nd_ntype(0),
                             nd_order(0), nd_warnbad(0), nd_lastupdate(0), nd_hierarchy_level(0),
                             nd_in_hierarchy(0), nd_ngpus(0), nd_gpus_real(false), nd_gpusn(NULL),
                             nd_ngpus_free(0), nd_ngpus_needed(0), nd_ngpus_to_be_used(0),
                             nd_gpustatus(NULL), nd_ngpustatus(0), nd_nmics(0),
                             nd_micstatus(NULL), nd_micjobs(NULL), nd_nmics_alloced(0),
                             nd_nmics_free(0), nd_nmics_to_be_used(0), parent(NULL),
                             num_node_boards(0), node_boards(NULL), numa_str(NULL),
                             gpu_str(NULL), nd_mom_reported_down(0), nd_is_alps_reporter(0),
                             nd_is_alps_login(0), alps_subnodes(NULL),
                             max_subnode_nppn(0), nd_power_state(0),
                             nd_power_state_change_time(0), nd_acl(NULL), nd_requestid(NULL)

  {
  struct addrinfo *pAddrInfo;

  this->nd_name            = new std::string(pname);
  this->nd_error           = new std::string("");
  this->nd_id              = node_mapper.get_new_id(this->get_name());
  this->nd_mom_port        = PBS_MOM_SERVICE_PORT;
  this->nd_mom_rm_port     = PBS_MANAGER_SERVICE_PORT;
  this->nd_addrs           = pul;       /* list of host byte order */
  this->nd_ntype           = NTYPE_CLUSTER;
  this->nd_needed          = 0;
  this->nd_order           = 0;
  this->nd_prop            = NULL;
  this->nd_status          = NULL;
  this->nd_note            = NULL;
  this->nd_state           = INUSE_DOWN;
  this->nd_first           = init_prop(pname);
  this->nd_last            = this->nd_first;
  this->nd_f_st            = init_prop(pname);
  this->nd_l_st            = this->nd_f_st;
  this->nd_hierarchy_level = -1; /* maximum unsigned short */
  this->nd_nprops          = 0;
  this->nd_nstatus         = 0;
  this->nd_warnbad         = 0;
  this->nd_ngpus           = 0;
  this->nd_gpustatus       = NULL;
  this->nd_ngpustatus      = 0;
  this->nd_ms_jobs         = new std::vector<std::string>();
  this->nd_acl             = NULL;
  this->nd_requestid       = new std::string();

  // NUMA nodes don't have their own address and their name is not in DNS.
  if (!skip_lookup)
    {
    if (pbs_getaddrinfo(pname,NULL,&pAddrInfo))
      {
      nd_error->append("Couldn't resolve hostname '");
      nd_error->append(pname);
      nd_error->append("'.");
      return;
      }

    memcpy(&this->nd_sock_addr,pAddrInfo->ai_addr,sizeof(struct sockaddr_in));
    }

  this->nd_mutex = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
  if (this->nd_mutex == NULL)
    {
    nd_error->append("Could not allocate memory for the node's mutex");
    log_err(ENOMEM, __func__, "Could not allocate memory for the node's mutex");
    }
  else
    pthread_mutex_init(this->nd_mutex, NULL);
  } // END pbsnode(char *, unsigned long *, bool)



pbsnode::pbsnode() : nd_first(NULL), nd_last(NULL), nd_f_st(NULL), nd_l_st(NULL), nd_addrs(NULL),
                     nd_prop(NULL), nd_status(NULL), nd_note(NULL), nd_flag(okay),
                     nd_mom_port(0), nd_mom_rm_port(0), nd_sock_addr(), nd_nprops(0), nd_slots(),
                     nd_job_usages(), nd_needed(0), nd_np_to_be_used(0), nd_state(0), nd_ntype(0),
                     nd_order(0), nd_warnbad(0), nd_lastupdate(0), nd_hierarchy_level(0),
                     nd_in_hierarchy(0), nd_ngpus(0), nd_gpus_real(false), nd_gpusn(NULL),
                     nd_ngpus_free(0), nd_ngpus_needed(0), nd_ngpus_to_be_used(0),
                     nd_gpustatus(NULL), nd_ngpustatus(0), nd_nmics(0), nd_micstatus(NULL),
                     nd_micjobs(NULL), nd_nmics_alloced(0), nd_nmics_free(0),
                     nd_nmics_to_be_used(0), parent(NULL), num_node_boards(0), node_boards(NULL),
                     numa_str(NULL), gpu_str(NULL), nd_mom_reported_down(0),
                     nd_is_alps_reporter(0), nd_is_alps_login(0),
                     alps_subnodes(NULL), max_subnode_nppn(0), nd_power_state(0),
                     nd_power_state_change_time(0), nd_acl(NULL), nd_requestid(NULL),
                     nd_mutex(NULL)

  {
  this->nd_mutex = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
  pthread_mutex_init(this->nd_mutex, NULL);

  this->nd_ms_jobs   = new std::vector<std::string>();
  this->nd_name = new std::string();
  this->nd_error = new std::string();
  } // END pbsnode()



pbsnode::pbsnode(
    
  const pbsnode &other) : 
                     nd_flag(other.nd_flag),
                     nd_mom_port(other.nd_mom_port), nd_mom_rm_port(other.nd_mom_rm_port),
                     nd_sock_addr(other.nd_sock_addr), nd_nprops(other.nd_nprops),
                     nd_slots(other.nd_slots), nd_job_usages(other.nd_job_usages),
                     nd_needed(other.nd_needed), nd_np_to_be_used(other.nd_np_to_be_used),
                     nd_state(other.nd_state), nd_ntype(other.nd_ntype), nd_order(other.nd_order),
                     nd_warnbad(other.nd_warnbad), nd_lastupdate(other.nd_lastupdate),
                     nd_hierarchy_level(other.nd_hierarchy_level),
                     nd_in_hierarchy(other.nd_in_hierarchy), nd_ngpus(other.nd_ngpus),
                     nd_gpus_real(other.nd_gpus_real),
                     nd_ngpus_free(other.nd_ngpus_free), nd_ngpus_needed(other.nd_ngpus_needed),
                     nd_ngpus_to_be_used(other.nd_ngpus_to_be_used),
                     nd_ngpustatus(other.nd_ngpustatus),
                     nd_nmics(other.nd_nmics),
                     nd_nmics_alloced(other.nd_nmics_alloced),
                     nd_nmics_free(other.nd_nmics_free), 
                     nd_nmics_to_be_used(other.nd_nmics_to_be_used),
                     num_node_boards(other.num_node_boards), 
                     nd_mom_reported_down(other.nd_mom_reported_down),
                     nd_is_alps_reporter(other.nd_is_alps_reporter),
                     nd_is_alps_login(other.nd_is_alps_login), nd_note(NULL),
                     max_subnode_nppn(other.max_subnode_nppn),
                     nd_power_state(other.nd_power_state), gpu_str(NULL), numa_str(NULL),
                     nd_power_state_change_time(other.nd_power_state_change_time),
                     nd_acl(NULL), nd_requestid(NULL),
                     nd_error(NULL)

  {
  memcpy(this->nd_mac_addr, other.nd_mac_addr, sizeof(this->nd_mac_addr));
  memcpy(this->nd_ttl, other.nd_ttl, sizeof(this->nd_ttl));

  this->nd_name    = new std::string(other.nd_name->c_str());
  this->nd_error   = new std::string(other.nd_error->c_str());
  this->parent     = other.parent;
  this->nd_ms_jobs   = new std::vector<std::string>();

  for (unsigned int i = 0; i < other.nd_ms_jobs->size(); i++)
    this->nd_ms_jobs->push_back(other.nd_ms_jobs->at(i));

  if (other.nd_gpusn == NULL)
    this->nd_gpusn = NULL;
  else
    {
    for (short i = 0; i < other.nd_ngpus; i++)
      this->create_a_gpusubnode();
    }
  
  this->nd_first = init_prop(this->nd_name->c_str());
  this->nd_last  = this->nd_first;
  this->nd_f_st  = init_prop(this->nd_name->c_str());
  this->nd_l_st  = this->nd_f_st;
  this->nd_micjobs = NULL;
  this->nd_prop = other.nd_prop;
  this->nd_status = NULL;
  if (other.nd_note != NULL)
    this->nd_note = strdup(nd_note);
  this->nd_gpustatus = NULL;
  this->nd_micstatus = NULL;

  if (other.numa_str != NULL)
    this->numa_str = strdup(other.numa_str);

  if (other.gpu_str != NULL)
    this->gpu_str = strdup(other.gpu_str);
  
  this->node_boards = NULL;
  this->nd_addrs = NULL; // FIX-ME

  if (other.nd_requestid != NULL)
    {
    this->nd_requestid = new std::string(other.nd_requestid->c_str());
    }

  this->nd_mutex = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
  pthread_mutex_init(this->nd_mutex, NULL);
 
  this->alps_subnodes = NULL;
  // copy nd_acl

  }



int pbsnode::get_node_id() const

  {
  return(this->nd_id);
  }



const char *pbsnode::get_name() const

  {
  return(this->nd_name->c_str());
  } // END get_node_name()



const char *pbsnode::get_error() const

  {
  return(this->nd_error->c_str());
  } // END get_error()



/*
 * @param ph - the list we're adding the output into
 * @param aname - the name of the jobs attribute
 */

int pbsnode::encode_jobs(

  tlist_head *ph,
  const char *aname) const

  {
  FUNCTION_TIMER
  svrattrl          *pal;

  bool               first = true;
  std::string        job_str;
  std::string        range_str;
  
  for (int i = 0; i < (int)this->nd_job_usages.size(); i++)
    {
    const job_usage_info &jui = this->nd_job_usages[i];

    populate_range_string_from_slot_tracker(jui.est, range_str);

    if (first == false)
      job_str += ",";

    job_str += range_str;
    job_str += "/";
    job_str += job_mapper.get_name(jui.internal_job_id);

    first = false;
    }

  if (first == true)
    {
    /* no jobs currently on this node */

    return(0);
    }

  pal = attrlist_create(aname, NULL, job_str.length() + 1);

  snprintf(pal->al_value, job_str.length() + 1, "%s", job_str.c_str());

  pal->al_flags = ATR_VFLAG_SET;

  append_link(ph, &pal->al_link, pal);

  return(0);                  /* success */
  }  /* END encode_jobs() */


job *pbsnode::get_job_from_job_usage_info(
    
  job_usage_info *jui)

  {
  job *pjob;

  this->unlock_node(__func__, NULL, LOGLEVEL);
  pjob = svr_find_job_by_id(jui->internal_job_id);
  this->lock_node(__func__, NULL, LOGLEVEL);

  return(pjob);
  }



int pbsnode::login_encode_jobs(

  tlist_head *phead)

  {
  job            *pjob;
  std::string     job_str = "";
  char            str_buf[MAXLINE*2];
  svrattrl       *pal;

  if (phead == NULL)
    {
    log_err(PBSE_BAD_PARAMETER, __func__, "NULL input tlist_head pointer");
    return(PBSE_BAD_PARAMETER);
    }

  for (unsigned int i = 0; i < this->nd_job_usages.size(); i++)
    {
    // must be a copy and not a reference to avoid crashes: get_job_usage_info()
    // potentially releases the node mutex meaning a reference could refer tp bad
    // info.
    job_usage_info  jui = this->nd_job_usages[i];
    int             jui_index;
    int             jui_iterator = -1;
    int             login_id = -1;

    pjob = this->get_job_from_job_usage_info(&jui);
    
    if (pjob != NULL)
      {
      login_id = pjob->ji_wattr[JOB_ATR_login_node_key].at_val.at_long;
      unlock_ji_mutex(pjob, __func__, "1", LOGLEVEL);
      }

    const char *job_id = NULL;
    
    while ((jui_index = jui.est.get_next_occupied_index(jui_iterator)) != -1)
      {
      if (job_id == NULL)
        job_id = job_mapper.get_name(jui.internal_job_id);

      if (this->get_node_id() != login_id)
        {
        if (job_str.length() != 0)
          snprintf(str_buf, sizeof(str_buf), ",%d/%s", jui_index, job_id);
        else
          snprintf(str_buf, sizeof(str_buf), "%d/%s", jui_index, job_id);

        job_str += str_buf;
        }
      }
    }

  if ((pal = attrlist_create((char *)ATTR_NODE_jobs, (char *)NULL, strlen(job_str.c_str()) + 1)) == NULL)
    {
    log_err(ENOMEM, __func__, "");
    return(ENOMEM);
    }

  strcpy((char *)pal->al_value, job_str.c_str());
  pal->al_flags = ATR_VFLAG_SET;

  append_link(phead, &pal->al_link, pal);

  return(PBSE_NONE);
  } /* END login_encode_jobs() */



/*
 * status_nodeattrib()
 *
 * @param pal - specific attributes we should report
 * @param padef - the defined node attributes
 * @param limit - the number of attributes in padef
 * @param priv - the requestor's privilege level
 * @param phead - the list we're encoding the attributes into
 * @param bad - the index of the element which caused an error
 */

int pbsnode::status_nodeattrib(

  svrattrl        *pal,
  attribute_def   *padef,
  int              limit,
  int              priv,
  tlist_head      *phead,
  int             *bad)

  {
  int   i;
  int   rc = 0;  /*return code, 0 == success*/
  int   index;
  int   nth;  /*tracks list position (ordinal tacker)   */

  pbs_attribute atemp[ND_ATR_LAST]; /*temporary array of attributes   */

  if ((padef == NULL) ||
      (bad == NULL) ||
      (phead == NULL))
    {
    rc = PBSE_BAD_PARAMETER;
    return(rc);
    }

  memset(&atemp, 0, sizeof(atemp));

  priv &= ATR_DFLAG_RDACC;    /* user-client privilege          */

  for (i = 0;i < ND_ATR_LAST;i++)
    {
    /*set up attributes using data from node*/
    if (i == ND_ATR_state)
      atemp[i].at_val.at_short = this->nd_state;
    else if (i == ND_ATR_power_state)
      atemp[i].at_val.at_short = this->nd_power_state;
    else if (i == ND_ATR_properties)
      atemp[i].at_val.at_arst = this->nd_prop;
    else if (i == ND_ATR_status)
      atemp[i].at_val.at_arst = this->nd_status;
    else if (i == ND_ATR_ntype)
      atemp[i].at_val.at_short = this->nd_ntype;
    else if (i == ND_ATR_ttl)
      atemp[i].at_val.at_str = (char *)this->nd_ttl;
    else if (i == ND_ATR_acl)
      atemp[i].at_val.at_arst = this->nd_acl;
    else if (i == ND_ATR_requestid)
      atemp[i].at_val.at_str = (char *)this->nd_requestid->c_str();
    else if (i == ND_ATR_jobs)
      atemp[i].at_val.at_jinfo = NULL;
    else if (i == ND_ATR_np)
      atemp[i].at_val.at_long = this->nd_slots.get_total_execution_slots();
    else if (i == ND_ATR_note)
      atemp[i].at_val.at_str  = this->nd_note;
    else if (i == ND_ATR_mom_port)
      atemp[i].at_val.at_long  = this->nd_mom_port;
    else if (i == ND_ATR_mom_rm_port)
      atemp[i].at_val.at_long  = this->nd_mom_rm_port;
    /* skip NUMA attributes */
    else if (i == ND_ATR_num_node_boards)
      continue;
    else if (i == ND_ATR_numa_str)
      continue;
    else if (i == ND_ATR_gpus_str)
      continue;
    else if (i == ND_ATR_gpustatus)
      atemp[i].at_val.at_arst = this->nd_gpustatus;
    else if (i == ND_ATR_gpus)
      {
      if (this->nd_ngpus == 0)
        continue;

      atemp[i].at_val.at_long  = this->nd_ngpus;
      }
      else if ((padef + i)->at_name != NULL)
      {
      if (!strcmp((padef + i)->at_name, ATTR_NODE_mics))
        {
        if (this->nd_nmics == 0)
          continue;

        atemp[i].at_val.at_long  = this->nd_nmics;
        }
      else if (!strcmp((padef + i)->at_name, ATTR_NODE_micstatus))
        atemp[i].at_val.at_arst = this->nd_micstatus;
      }
    else
      {
      /*we don't ever expect this*/
      *bad = 0;

      return(PBSE_UNKNODEATR);
      }

    atemp[i].at_flags = ATR_VFLAG_SET; /*artificially set the value's flags*/
    }

  if (pal != NULL)
    {
    /*caller has requested status on specific node-attributes*/
    nth = 0;

    while (pal != NULL)
      {
      ++nth;

      index = find_attr(padef, pal->al_name, limit);

      if (index < 0)
        {
        *bad = nth;  /*name in this position can't be found*/

        rc = PBSE_UNKNODEATR;

        break;
        }

      if ((padef + index)->at_flags & priv)
        {
        if (index == ND_ATR_jobs)
          {
          if (this->nd_is_alps_login == TRUE)
            rc = this->login_encode_jobs(phead);
          else
            rc = this->encode_jobs(phead, (padef + index)->at_name);
          }
        else
          {
          if (index == ND_ATR_status)
            atemp[index].at_val.at_arst = this->nd_status;

          rc = ((padef + index)->at_encode(
                &atemp[index],
                phead,
                (padef + index)->at_name,
                NULL,
                ATR_ENCODE_CLIENT,
                0));
          }

        if (rc < 0)
          {
          rc = -rc;

          break;
          }
        else
          {
          /* encoding was successful */

          rc = 0;
          }
        }

      pal = (svrattrl *)GET_NEXT(pal->al_link);
      }  /* END while (pal != NULL) */
    }    /* END if (pal != NULL) */
  else
    {
    /* non-specific request, return all readable attributes */
    for (index = 0; index < limit; index++)
      {
      if (index == ND_ATR_jobs)
        {
        if (this->nd_is_alps_login == true)
          rc = this->login_encode_jobs(phead);
        else
          rc = this->encode_jobs(phead, (padef + index)->at_name);
        }
      else if (((padef + index)->at_flags & priv) &&
               !((padef + index)->at_flags & ATR_DFLAG_NOSTAT))
        {
        if (index == ND_ATR_status)
          atemp[index].at_val.at_arst = this->nd_status;

        rc = (padef + index)->at_encode(
               &atemp[index],
               phead,
               (padef + index)->at_name,
               NULL,
               ATR_ENCODE_CLIENT,
               0);

        if (rc < 0)
          {
          rc = -rc;

          break;
          }
        else
          {
          /* encoding was successful */

          rc = 0;
          }
        }
      }    /* END for (index) */
    }      /* END else (pal != NULL) */

  return(rc);
  }  /* END status_nodeattrib() */



int pbsnode::lock_node(
    
  const char     *id,
  const char     *msg,
  int             logging)

  {
  int   rc = PBSE_NONE;
  char  err_msg[MAXLINE];
  char  stub_msg[] = "no pos";
  
  if (logging >= 10)
    {
    if (msg == NULL)
      msg = stub_msg;
    snprintf(err_msg, sizeof(err_msg),
      "locking start %s in method %s-%s", this->get_name(), id, msg);
    log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, err_msg);
    }
  
  if (pthread_mutex_lock(this->nd_mutex) != 0)
    {
    if (logging >= 10)
      {
      snprintf(err_msg, sizeof(err_msg), "ALERT: cannot lock node %s mutex in method %s",
          this->get_name(), id);
      log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, err_msg);
      }
    rc = PBSE_MUTEX;
    }
  
  if (logging >= 10)
    {
    snprintf(err_msg, sizeof(err_msg),
      "locking complete %s in method %s", this->get_name(), id);
    log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, err_msg);
    }


/* We may need to add this functionality to lock node and unlock node
int tmp_lock_node(

  struct pbsnode *the_node,
  const char     *id,
  const char     *msg,
  int             logging)
  {
  int rc = the_node->lock_node(id, msg, logging);
  if (rc == PBSE_NONE)
    {
    the_node->nd_tmp_unlock_count--;
    }
  return(rc);
  }


int tmp_unlock_node(

  struct pbsnode *the_node,
  const char     *id,
  const char     *msg,
  int             logging)
  {
  the_node->nd_tmp_unlock_count++;
  return(the_node->unlock_node(id, msg, logging));
  } */

  return rc;
  } /* END lock_node() */



int pbsnode::unlock_node(
    
  const char     *id,
  const char     *msg,
  int             logging)

  {
  int   rc = PBSE_NONE;
  char  err_msg[MAXLINE];
  char  stub_msg[] = "no pos";

  if (logging >= 10)
    {
    if (msg == NULL)
      msg = stub_msg;
    snprintf(err_msg, sizeof(err_msg),
      "unlocking %s in method %s-%s", this->get_name(), id, msg);
    log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, err_msg);
    }

  if (pthread_mutex_unlock(this->nd_mutex) != 0)
    {
    if (logging >= 10)
      {
      snprintf(err_msg, sizeof(err_msg),
        "ALERT: cannot unlock node %s mutex in method %s", this->get_name(), id);
      log_record(PBSEVENT_DEBUG, PBS_EVENTCLASS_NODE, __func__, err_msg);
      }

    rc = PBSE_MUTEX;
    }

  return rc;
  } /* END unlock_node() */



short pbsnode::get_mic_count() const

  {
  return(this->nd_nmics);
  } // END get_mic_count()



void pbsnode::set_mic_count(

  short new_mics)

  {
  short old_mics = this->nd_nmics;
  this->nd_nmics = new_mics;

  if (new_mics > old_mics)
    {
    this->nd_nmics_free += new_mics - old_mics;

    if (new_mics > this->nd_nmics_alloced)
      {
      struct jobinfo *tmp = (struct jobinfo *)calloc(new_mics, sizeof(struct jobinfo));

      if (tmp != NULL)
        {
        memcpy(tmp, this->nd_micjobs, sizeof(struct jobinfo) * this->nd_nmics_alloced);
        free(this->nd_micjobs);
        this->nd_micjobs = tmp;

        for (int i = this->nd_nmics_alloced; i < new_mics; i++)
          this->nd_micjobs[i].internal_job_id = -1;

        this->nd_nmics_alloced = new_mics;
        }
      }
    }

  } // END set_mic_count()



bool pbsnode::node_is_spec_acceptable(

  single_spec_data *spec,
  char             *ProcBMStr,
  int              *eligible_nodes,
  bool              job_is_exclusive,
  int               requested_gpu_mode) const

  {
  struct prop    *prop = spec->prop;

  int             ppn_req = spec->ppn;
  int             gpu_req = spec->gpu;
  int             mic_req = spec->mic;
  int             gpu_free;
  int             np_free;
  int             mic_free;

#ifdef GEOMETRY_REQUESTS
  if (IS_VALID_STR(ProcBMStr))
    {
    if ((this->nd_state != INUSE_FREE)||(this->nd_power_state != POWER_STATE_RUNNING))
      return(false);

    if (node_satisfies_request(this, ProcBMStr) == FALSE)
      return(false);
    }
#endif

  /* make sure that the node has properties */
  if (this->has_prop(prop) == false)
    return(false);

  if ((this->has_ppn(ppn_req, SKIP_NONE) == false) ||
      (this->gpu_count(false, requested_gpu_mode) < gpu_req) ||
      (this->get_mic_count() < mic_req))
    return(false);

  (*eligible_nodes)++;

  if (((this->nd_state & (INUSE_OFFLINE | INUSE_DOWN | INUSE_RESERVE | INUSE_JOB)) != 0) ||
      (this->nd_power_state != POWER_STATE_RUNNING))
    return(false);

  gpu_free = this->gpu_count(true, requested_gpu_mode) - this->nd_ngpus_to_be_used;
  np_free  = this->nd_slots.get_number_free() - this->nd_np_to_be_used;
  mic_free = this->nd_nmics_free - this->nd_nmics_to_be_used;
  
  if ((ppn_req > np_free) ||
      (gpu_req > gpu_free) ||
      (mic_req > mic_free))
    return(false);

  if (job_is_exclusive)
    {
    if (this->nd_slots.get_number_free() != this->nd_slots.get_total_execution_slots())
      {
      return false;
      }
    }

  return(true);
  } /* END node_is_spec_acceptable() */



/*
** Look through the property list and make sure that all
** those marked are contained in the node.
*/

bool pbsnode::has_prop(

  struct prop *props) const

  {
  struct prop *need;

  for (need = props; need != NULL; need = need->next)
    {
    struct prop *pp;

    if (need->mark == 0) /* not marked, skip */
      continue;

    for (pp = this->nd_first;pp != NULL;pp = pp->next)
      {
      if (strcmp(pp->name, need->name) == 0)
        break;
      }

    if (pp == NULL)
      return(false);
    }

  return(true);
  }  /* END has_prop() */



/*
 * has_ppn()
 *
 *
 * @param node_req - the number requested
 * @param free - the way we want this to be compared
 * @return true if there are sufficient execution slots for the request
 */

bool pbsnode::has_ppn(
  
  int node_req,
  int free) const

  {
  if ((free != SKIP_NONE) &&
      (free != SKIP_NONE_REUSE) &&
      (this->nd_slots.get_number_free() >= node_req))
    {
    return(true);
    }

  if ((free == SKIP_NONE) && 
      (this->nd_slots.get_total_execution_slots() >= node_req))
    {
    return(true);
    }

  return(false);
  }  /* END has_ppn() */



int pbsnode::gpu_count(

  bool freeonly,
  int  requested_gpu_mode) const

  {
  int  count = 0;
  char log_buf[LOCAL_LOG_BUF_SIZE];

  if ((this->nd_state & INUSE_OFFLINE) ||
      (this->nd_state & INUSE_UNKNOWN) ||
      (this->nd_state & INUSE_DOWN)||
      (this->nd_power_state != POWER_STATE_RUNNING))
    {
    if (LOGLEVEL >= 7)
      {
      sprintf(log_buf,
        "Counted %d gpus %s on node %s that was skipped",
        count,
        (freeonly ? "free":"available"),
        this->get_name());
    
      log_ext(-1, __func__, log_buf, LOG_DEBUG);
      }

    return(count);
    }

  if (this->nd_gpus_real)
    {
    for (int j = 0; j < this->nd_ngpus; j++)
      {
      struct gpusubn *gn = this->nd_gpusn + j;

      /* always ignore unavailable gpus */
      if (gn->state == gpu_unavailable)
        continue;

      if (!freeonly)
        {
        count++;
        }
      else if ((gn->state == gpu_unallocated) ||
               ((gn->state == gpu_shared) &&
                (requested_gpu_mode == gpu_normal)))
        {
        count++;;
        }
      }
    }
  else
    {
    /* virtual gpus */
    if (freeonly)
      {
      count = this->nd_ngpus_free;
      }
    else
      {
      count = this->nd_ngpus;
      }
    }

  if (LOGLEVEL >= 7)
    {
    sprintf(log_buf,
      "Counted %d gpus %s on node %s",
      count,
      (freeonly ? "free":"available"),
      this->get_name());

    log_ext(-1, __func__, log_buf, LOG_DEBUG);
    }

  return(count);
  }  /* END gpu_count() */



void pbsnode::abort_request(

  node_job_add_info *naji)

  {
  this->nd_np_to_be_used    -= naji->ppn_needed;
  this->nd_ngpus_to_be_used -= naji->gpu_needed;
  this->nd_nmics_to_be_used -= naji->mic_needed;
  }



int pbsnode::add_job_to_mic(

  int  index,
  job *pjob)

  {
  int rc = -1;

  if (this->nd_micjobs[index].internal_job_id == -1)
    {
    this->nd_micjobs[index].internal_job_id = pjob->ji_internal_id;
    this->nd_nmics_free--;
    this->nd_nmics_to_be_used--;
    rc = PBSE_NONE;
    }

  return(rc);
  } /* END add_job_to_mic() */



void pbsnode::remove_job_from_nodes_mics(

  job *pjob)

  {
  for (short i = 0; i < this->get_mic_count(); i++)
    {
    if (this->nd_micjobs[i].internal_job_id == pjob->ji_internal_id)
      this->nd_micjobs[i].internal_job_id = -1;
    }
  } /* END remove_job_from_nodes_mics() */



int pbsnode::get_execution_slot_count() const

  {
  return(this->nd_slots.get_total_execution_slots());
  }



int pbsnode::get_execution_slot_free_count() const

  {
  return(this->nd_slots.get_number_free());
  }



int pbsnode::add_execution_slot()

  {
  this->nd_slots.add_execution_slot();

  if ((this->nd_state & INUSE_JOB) != 0)
    this->nd_state &= ~INUSE_JOB;

  return(PBSE_NONE);
  }  /* END add_execution_slot() */



short pbsnode::get_service_port() const

  {
  return(this->nd_mom_port);
  }



short pbsnode::get_manager_port() const

  {
  return(this->nd_mom_rm_port);
  }



void pbsnode::set_service_port(

  short port)

  {
  this->nd_mom_port = port;
  }



void pbsnode::set_manager_port(

  short port)

  {
  this->nd_mom_rm_port = port;
  }



void pbsnode::delete_a_subnode()

  {
  this->nd_slots.remove_execution_slot();
  }


int pbsnode::set_execution_slot_count(

  int esc)

  {
  int old_np = this->nd_slots.get_total_execution_slots();
  int new_np = esc;
      
  if (new_np <= 0)
    return(PBSE_BADATVAL);

  while (new_np != old_np)
    {
    if (new_np < old_np)
      {
      this->delete_a_subnode();
      old_np--;
      }
    else
      {
      this->add_execution_slot();
      old_np++;
      }
    }
  }



bool pbsnode::is_slot_occupied(

  int index) const

  {
  return(this->nd_slots.is_occupied(index));
  }



int pbsnode::reserve_execution_slots(

  int                     num_to_reserve,
  execution_slot_tracker &subset)

  {
  return(this->nd_slots.reserve_execution_slots(num_to_reserve, subset));
  }


int pbsnode::unreserve_execution_slots(

  const execution_slot_tracker &subset)

  {
  return(this->nd_slots.unreserve_execution_slots(subset));
  }



int pbsnode::mark_slot_as_used(

  int index)

  {
  return(this->nd_slots.mark_as_used(index));
  }



int pbsnode::mark_slot_as_free(

  int index)

  {
  return(this->nd_slots.mark_as_free(index));
  }



/* 
 * update_node_state - central location for updating node state
 * NOTE:  called each time a node is marked down, each time a MOM reports node  
 *        status, and when pbs_server sends hello/cluster_addrs
 *
 * @param newstate - the new state, one of INUSE_*
 **/


void pbsnode::update_node_state(

  int newstate)

  {
  char            log_buf[LOCAL_LOG_BUF_SIZE];

  /* No need to do anything if newstate == oldstate */
  if (this->nd_state == newstate)
    return;

  /*
   * LOGLEVEL >= 4 logs all state changes
   *          >= 2 logs down->(busy|free) changes
   *          (busy|free)->down changes are always logged
   */

  if (LOGLEVEL >= 4)
    {
    snprintf(log_buf, sizeof(log_buf), "adjusting state for node %s - state=%d, newstate=%d",
      this->nd_name->c_str(),
      this->nd_state,
      newstate);

    log_record(PBSEVENT_SCHED, PBS_EVENTCLASS_REQUEST, __func__, log_buf);
    }

  if (newstate & INUSE_DOWN)
    {
    if (!(this->nd_state & INUSE_DOWN))
      {
      snprintf(log_buf, sizeof(log_buf), "node %s marked down", this->get_name());

      this->nd_state |= INUSE_DOWN;
      this->nd_state &= ~INUSE_UNKNOWN;
      }

    /* ignoring the obvious possibility of a "down,busy" node */
    }  /* END if (newstate & INUSE_DOWN) */
  else if (newstate & INUSE_BUSY)
    {
    if ((!(this->nd_state & INUSE_BUSY) && (LOGLEVEL >= 4)) ||
        ((this->nd_state & INUSE_DOWN) && (LOGLEVEL >= 2)))
      {
      sprintf(log_buf, "node %s marked busy",
        this->get_name());
      }

    this->nd_state |= INUSE_BUSY;

    this->nd_state &= ~INUSE_UNKNOWN;

    if (this->nd_state & INUSE_DOWN)
      {
      this->nd_state &= ~INUSE_DOWN;
      }
    }  /* END else if (newstate & INUSE_BUSY) */
  else if (newstate == INUSE_FREE)
    {
    if (((this->nd_state & INUSE_DOWN) && (LOGLEVEL >= 2)) ||
        ((this->nd_state & INUSE_BUSY) && (LOGLEVEL >= 4)))
      {
      sprintf(log_buf, "node %s marked free",
        this->get_name());
      }

    this->nd_state &= ~INUSE_BUSY;

    this->nd_state &= ~INUSE_UNKNOWN;

    if (this->nd_state & INUSE_DOWN)
      {
      this->nd_state &= ~INUSE_DOWN;
      }
    }    /* END else if (newstate == INUSE_FREE) */

  if (newstate & INUSE_UNKNOWN)
    {
    this->nd_state |= INUSE_UNKNOWN;
    }

  if ((LOGLEVEL >= 2) && (log_buf[0] != '\0'))
    {
    log_record(PBSEVENT_SCHED, PBS_EVENTCLASS_REQUEST, __func__, log_buf);
    }
  }  /* END update_node_state() */



bool pbsnode::check_node_for_job(

  int internal_job_id) const

  {
  for (int i = 0; i < (int)this->nd_job_usages.size(); i++)
    {
    const job_usage_info &jui = this->nd_job_usages[i];

    if (internal_job_id == jui.internal_job_id)
      return(true);
    }

  /* not found */
  return(false);
  } /* END check_node_for_job() */



int pbsnode::setup_node_boards()

  {
  pbsnode *pn;
  char     pname[MAX_LINE];
  char    *np_ptr = NULL;
  char    *gp_ptr = NULL;
  int      np;
  int      gpus;
  int      rc = PBSE_NONE;

  char     log_buf[LOCAL_LOG_BUF_SIZE];

  this->parent = NULL;

  /* if this isn't a numa node, return no error */
  if ((this->num_node_boards == 0) &&
      (this->numa_str == NULL))
    {
    return(PBSE_NONE);
    }

  /* determine the number of cores per node */
  if (this->numa_str != NULL)
    {
    np_ptr = this->numa_str;
    }
  else
    np = this->get_execution_slot_count() / this->num_node_boards;

  /* determine the number of gpus per node */
  if (this->gpu_str != NULL)
    {
    gp_ptr = this->gpu_str;
    read_val_and_advance(&gpus,&gp_ptr);
    }
  else
    gpus = this->gpu_count() / this->num_node_boards;

  for (int i = 0; i < this->num_node_boards; i++)
    {
    /* each numa node just has a number for a name */
    snprintf(pname,sizeof(pname),"%s-%d",
      this->get_name(), i);

    pn = new pbsnode(pname, this->nd_addrs, true);

    if (strlen(pn->get_error()) > 0)
      {
      delete pn;
      return(rc);
      }

    /* make sure the server communicates on the correct ports */
    pn->set_service_port(this->get_service_port());
    pn->set_manager_port(this->get_manager_port());
    memcpy(&pn->nd_sock_addr, &this->nd_sock_addr, sizeof(pn->nd_sock_addr));

    /* update the np string pointer */
    if (np_ptr != NULL)
      read_val_and_advance(&np, &np_ptr);

    /* create the subnodes for this node */
    for (int j = 0; j < np; j++)
      pn->add_execution_slot();

    /* create the gpu subnodes for this node */
    for (int j = 0; j < gpus; j++)
      {
      if (pn->create_a_gpusubnode() != PBSE_NONE)
        {
        /* ERROR */
        free(pn);
        return(PBSE_SYSTEM);
        }
      }

    /* update the gpu string pointer */
    if (gp_ptr != NULL)
      read_val_and_advance(&gpus,&gp_ptr);

    copy_properties(pn, this);

    /* add the node to the private tree */
    this->node_boards = AVL_insert(i,
        pn->get_service_port(),
        pn,
        this->node_boards);

    /* set my parent node pointer */
    pn->parent = this;
    } /* END for each node_board */

  if (LOGLEVEL >= 3)
    {
    snprintf(log_buf,sizeof(log_buf),
      "Successfully created %d numa nodes for node %s\n",
      this->num_node_boards,
      this->get_name());

    log_event(PBSEVENT_SYSTEM, PBS_EVENTCLASS_NODE, __func__, log_buf);
    }

  return(PBSE_NONE);
  } /* END setup_node_boards() */




/*
 * is_job_on_node - return TRUE if this internal job id is present on this
 */

bool pbsnode::is_job_on_node(

  int             internal_job_id) const

  {
  struct pbsnode *numa;

  int             present = FALSE;
  int             i;

  if (this->num_node_boards > 0)
    {
    /* check each subnode on each numa node for the job */
    for (i = 0; i < this->num_node_boards; i++)
      {
      numa = AVL_find(i, this->get_service_port(), this->node_boards);

      numa->lock_node(__func__, "before check_node_for_job numa", LOGLEVEL);
      present = this->check_node_for_job(internal_job_id);
      numa->unlock_node(__func__, "after check_node_for_job numa", LOGLEVEL);

      /* leave loop if we found the job */
      if (present != FALSE)
        break;
      } /* END for each numa node */
    }
  else
    {
    present = this->check_node_for_job(internal_job_id);
    }

  return(present);
  }  /* END is_job_on_node() */



/*
 **     reset gpu data in case mom reconnects with changed gpus.
 **     If we have real gpus, not virtual ones, then clear out gpu_status,
 **     gpus count and remove gpu subnodes.
 */

void pbsnode::clear_nvidia_gpus()

  {
  pbs_attribute   temp;

  if ((this->nd_gpus_real) &&
      (this->nd_ngpus > 0))
    {
    /* delete gpusubnodes by freeing it */
    free(this->nd_gpusn);
    this->nd_gpusn = NULL;

    /* reset # of gpus, etc */
    this->nd_ngpus = 0;
    this->nd_ngpus_free = 0;

    /* unset "gpu_status" node attribute */

    memset(&temp, 0, sizeof(temp));

    if (decode_arst(&temp, NULL, NULL, NULL, 0))
      {
      log_err(-1, __func__, "clear_nvidia_gpus:  cannot initialize attribute\n");

      return;
      }

    node_gpustatus_list(&temp, this, ATR_ACTION_ALTER);
    }

  return;
  }  /* END clear_nvidia_gpus() */



void pbsnode::set_node_down_if_inactive(

  time_t time_now,
  long   chk_len)

  {
  if (!(this->nd_state & INUSE_DOWN))
    {
    if (this->nd_lastupdate < (time_now - chk_len)) 
      {
      if (LOGLEVEL >= 0)
        {
        char log_buf[LOCAL_LOG_BUF_SIZE];
        sprintf(log_buf, "node %s not detected in %ld seconds, marking node down",
          this->get_name(),
          (long int)(time_now - this->nd_lastupdate));
        
        log_event(PBSEVENT_ADMIN, PBS_EVENTCLASS_SERVER, __func__, log_buf);
        }
      
      this->update_node_state(INUSE_DOWN);    
      }
    }
  }



int pbsnode::get_node_state() const

  {
  return(this->nd_state);
  }



int pbsnode::get_node_power_state() const

  {
  return(this->nd_power_state);
  }



const char *pbsnode::get_node_note() const

  {
  return(this->nd_note);
  }



void pbsnode::set_np_to_be_used(

  int np)

  {
  this->nd_np_to_be_used = np;
  }



/*
 * gpu_entry_by_id()
 *
 * get gpu index for this gpuid
 */

int pbsnode::gpu_entry_by_id(

  const char     *gpuid,
  bool            get_empty)

  {
  if (this->nd_gpus_real)
    {
    for (int j = 0; j < this->nd_ngpus; j++)
      {
      struct gpusubn *gn = this->nd_gpusn + j;

      if ((gn->gpuid != NULL) &&
          (!strcmp(gpuid, gn->gpuid)))
        return(j);
      }
    }

  /*
   * we did not find the entry.  if get_empty is set then look for an empty
   * slot.  If none is found then try to add a new entry to nd_gpusn
   */

  if (get_empty)
    {
    for (int j = 0; j < this->nd_ngpus; j++)
      {
      struct gpusubn *gn = this->nd_gpusn + j;

      if (gn->gpuid == NULL)
        {
        return(j);
        }
      }

    this->create_a_gpusubnode();
    return(this->nd_ngpus - 1);    
    }

  return(-1);
  }  /* END gpu_entry_by_id() */



int pbsnode::create_a_gpusubnode()

  {
  int rc = PBSE_NONE;
  struct gpusubn *tmp = NULL;
  
  tmp = (struct gpusubn *)calloc((1 + this->nd_ngpus), sizeof(struct gpusubn));

  if (tmp == NULL)
    {
    rc = PBSE_MEM_MALLOC;
    log_err(rc,__func__,
        (char *)"Couldn't allocate memory for a subnode. EPIC FAILURE");
    return(rc);
    }

  if (this->nd_ngpus > 0)
    {
    /* copy old memory to the new place */
    memcpy(tmp,this->nd_gpusn,(sizeof(struct gpusubn) * this->nd_ngpus));
    }

  /* now use the new memory */
  free(this->nd_gpusn);
  this->nd_gpusn = tmp;

  /* initialize the node */
  this->nd_gpus_real = false;
  this->nd_gpusn[this->nd_ngpus].inuse = FALSE;
  this->nd_gpusn[this->nd_ngpus].job_internal_id = -1;
  this->nd_gpusn[this->nd_ngpus].mode = gpu_normal;
  this->nd_gpusn[this->nd_ngpus].state = gpu_unallocated;
  this->nd_gpusn[this->nd_ngpus].flag = okay;
  this->nd_gpusn[this->nd_ngpus].index = this->nd_ngpus;
  this->nd_gpusn[this->nd_ngpus].gpuid = NULL;

  /* increment the number of gpu subnodes and gpus free */
  this->nd_ngpus++;
  this->nd_ngpus_free++;

  return(rc);
  } /* END create_a_gpusubnode() */



void pbsnode::set_order(

  int rank)

  {
  this->nd_order = rank;
  }
  


void pbsnode::save_space_for_req(
    
  single_spec_data *req)

  {
  this->nd_np_to_be_used += req->ppn;
  this->nd_ngpus_to_be_used += req->gpu;
  this->nd_nmics_to_be_used += req->mic;
  }


bool pbsnode::is_alps_login() const

  {
  return(this->nd_is_alps_login != 0);
  }


pbsnode *pbsnode::get_parent()

  {
  return(this->parent);
  }



int pbsnode::add_job_to_gpu_subnode(
    
  struct gpusubn *gn,
  job            *pjob)

  {
  if (!this->nd_gpus_real)
    {
    /* update the gpu subnode */
    gn->job_internal_id = pjob->ji_internal_id;
    gn->inuse = TRUE;

    /* update the main node */
    this->nd_ngpus_free--;
    }

  gn->job_count++;
  this->nd_ngpus_to_be_used--;

  return(PBSE_NONE);
  } /* END add_job_to_gpu_subnode() */



void pbsnode::set_real_gpu_mode(
    
  struct gpusubn *gn,
  int             requested_gpu_mode,
  const char     *jobid)

  {
  char log_buf[LOCAL_LOG_BUF_SIZE];

  if ((gn->mode == gpu_exclusive_thread) ||
      (gn->mode == gpu_exclusive_process) ||
      ((gn->mode == gpu_normal) && 
       ((requested_gpu_mode == gpu_exclusive_thread) ||
        (requested_gpu_mode == gpu_exclusive_process))))
    {
    /*
     * If this a real gpu in exclusive/single job mode, or a gpu in default
     * mode and the job requested an exclusive mode then we change state
     * to exclusive so we cannot assign another job to it
     */
    gn->state = gpu_exclusive;
    
    if (LOGLEVEL >= 7)
      {
      sprintf(log_buf,
        "Setting gpu %s/%d to state EXCLUSIVE for job %s",
        this->get_name(),
        gn->index,
        jobid);
    
      log_ext(-1, __func__, log_buf, LOG_DEBUG);
      }
    }
  else if ((gn->mode == gpu_normal) &&
      (requested_gpu_mode == gpu_normal) &&
      (gn->state == gpu_unallocated))
    {
    /*
     * If this a real gpu in shared/default job mode and the state is
     * unallocated then we change state to shared so only other shared jobs
     * can use it
     */
  
    gn->state = gpu_shared;
    
    if (LOGLEVEL >= 7)
      {
      sprintf(log_buf,
        "Setting gpu %s/%d to state SHARED for job %s",
        this->get_name(),
        gn->index,
        jobid);
    
      log_ext(-1, __func__, log_buf, LOG_DEBUG);
      }
    }
  } // END set_real_gpu_mode()



bool pbsnode::can_use_gpu(
    
  struct gpusubn *gn,
  int             requested_gpu_mode) const

  {
  if (this->nd_gpus_real)
    {
    if ((gn->state == gpu_unavailable) ||
        (gn->state == gpu_exclusive) ||
        ((((int)gn->mode == gpu_normal)) &&
         (requested_gpu_mode != gpu_normal) &&
         (gn->state != gpu_unallocated)))
      return(false);
    }
  else
    {
    if ((gn->state == gpu_unavailable) ||
        (gn->inuse == TRUE))
      return(false);
    }

  if ((gn->state == gpu_unavailable) ||
      ((gn->state == gpu_exclusive) && this->nd_gpus_real) ||
      ((this->nd_gpus_real) &&
       ((int)gn->mode == gpu_normal) &&
       ((requested_gpu_mode != gpu_normal) && (gn->state != gpu_unallocated))) ||
      ((!this->nd_gpus_real) && 
       (gn->inuse == TRUE)))
    return(false);

  return(true);
  } // END can_use_gpu()



int pbsnode::get_gpu_index(

  job *pjob,
  int  requested_gpu_mode)

  {
  struct gpusubn *gn;

  char            log_buf[LOCAL_LOG_BUF_SIZE];
  int             index = -1;

  /* place the gpus in the hostlist as well */
  for (int j = 0; j < this->nd_ngpus; j++)
    {
    gn = this->nd_gpusn + j;

    if (this->can_use_gpu(gn, requested_gpu_mode) == false)
      continue;

    // We have found a gpu we'll use
    
    this->add_job_to_gpu_subnode(gn, pjob);
    index = gn->index;

    if (LOGLEVEL >= 7)
      {
      sprintf(log_buf,
        "ADDING gpu %s/%d to exec_gpus still need %d",
        this->get_name(),
        j,
        this->nd_ngpus_needed);
    
      log_ext(-1, __func__, log_buf, LOG_DEBUG);
      }
    
    if (this->nd_gpus_real)
      this->set_real_gpu_mode(gn, requested_gpu_mode, pjob->ji_qs.ji_jobid);

    break;
    }

  return(index);
  } // END get_gpu_index()



short pbsnode::gpu_count() const

  {
  return(this->nd_ngpus);
  }



void pbsnode::delete_a_gpusubnode()

  {
  if (this->nd_ngpus < 1)
    {
    /* ERROR, can't free non-existent subnodes */
    return;
    }
  
  struct gpusubn *tmp = this->nd_gpusn + (this->nd_ngpus - 1);

  if (tmp->inuse == FALSE)
    this->nd_ngpus_free--;

  /* decrement the number of gpu subnodes */
  this->nd_ngpus--;

  if (this->nd_ngpus == 0)
    this->nd_gpusn = NULL;
  } /* END delete_a_gpusubnode() */



int pbsnode::set_gpu_count(

  short gpu_count)

  {
  short old_gp = this->nd_ngpus;

  if (gpu_count <= 0)
    return(PBSE_BADATVAL);

  while (gpu_count != old_gp)
    {

    if (gpu_count < old_gp)
      {
      this->delete_a_gpusubnode();
      old_gp--;
      }
    else
      {
      this->create_a_gpusubnode();
      old_gp++;
      }
    }
  }



void pbsnode::remove_job_from_nodes_gpus(

  job *pjob)

  {
  struct gpusubn *gn;
  char           *gpu_str = NULL;
  int             i;
  char            log_buf[LOCAL_LOG_BUF_SIZE];
  std::string     tmp_str;
  char            num_str[6];
 
  if (pjob->ji_wattr[JOB_ATR_exec_gpus].at_flags & ATR_VFLAG_SET)
    gpu_str = pjob->ji_wattr[JOB_ATR_exec_gpus].at_val.at_str;

  if (gpu_str != NULL)
    {
    /* reset gpu nodes */
    for (i = 0; i < this->nd_ngpus; i++)
      {
      gn = this->nd_gpusn + i;
      
      if (this->nd_gpus_real)
        {
        /* reset real gpu nodes */
        tmp_str = this->get_name();
        tmp_str += "-gpu/";
        sprintf (num_str, "%d", i);
        tmp_str += num_str;
        
        /* look thru the string and see if it has this host and gpuid.
         * exec_gpus string should be in format of 
         * <hostname>-gpu/<index>[+<hostname>-gpu/<index>...]
         *
         * if we are using the gpu node exclusively or if shared mode and
         * this is last job assigned to this gpu then set it's state
         * unallocated so its available for a new job. Takes time to get the
         * gpu status report from the moms.
         */
        
        if (strstr(gpu_str, tmp_str.c_str()) != NULL)
          {
          gn->job_count--;
          
          if ((gn->mode == gpu_exclusive_thread) ||
              (gn->mode == gpu_exclusive_process) ||
              ((gn->mode == gpu_normal) && 
               (gn->job_count == 0)))
            {
            gn->state = gpu_unallocated;
            
            if (LOGLEVEL >= 7)
              {
              sprintf(log_buf, "freeing node %s gpu %d for job %s",
                this->get_name(),
                i,
                pjob->ji_qs.ji_jobid);
              
              log_record(PBSEVENT_SCHED, PBS_EVENTCLASS_REQUEST, __func__, log_buf);
              }
            
            }
          }
        }
      else
        {
        if (gn->job_internal_id == pjob->ji_internal_id)
          {
          gn->inuse = FALSE;
          gn->job_internal_id = -1;
          
          this->nd_ngpus_free++;
          }
        }
      }
    }

  } /* END remove_job_from_nodes_gpus() */



void pbsnode::remove_job_from_node(

  int internal_job_id)

  {
  FUNCTION_TIMER
  char log_buf[LOCAL_LOG_BUF_SIZE];

  for (int i = 0; i < (int)this->nd_job_usages.size(); i++)
    {
    const job_usage_info &jui = this->nd_job_usages[i];

    if (jui.internal_job_id == internal_job_id)
      {
      this->unreserve_execution_slots(jui.est);
      this->nd_job_usages.erase(this->nd_job_usages.begin() + i);

      if (LOGLEVEL >= 6)
        {
        sprintf(log_buf, "increased execution slot free count to %d of %d\n",
          this->get_execution_slot_free_count(),
          this->get_execution_slot_count());
        
        log_record(PBSEVENT_SCHED, PBS_EVENTCLASS_REQUEST, __func__, log_buf);
        }

      this->nd_state &= ~INUSE_JOB;

      i--; /* the array has shrunk by 1 so we need to reduce i by one */
      }
    }
  
  } /* END remove_job_from_node() */



bool pbsnode::has_real_gpus() const

  {
  return(this->nd_gpus_real);
  }


void pbsnode::set_real_gpus()

  {
  this->nd_gpus_real = true;
  }



bool pbsnode::can_set_gpu_mode(

  const char  *gpuid,
  int          gpumode,
  std::string &err_msg)

  {
  int gpuidx;

  /* validate that the node has real gpus not virtual */
  if (!this->has_real_gpus())
    {
    err_msg = "Cannot set mode for non-real gpus.";
    return(false);
    }

  /* validate the gpuid exists */
  if ((gpuidx = this->gpu_entry_by_id(gpuid, false)) == -1)
    {
    err_msg = "GPU ID does not exist on node";
    return(false);
    }

  /* for mode changes validate the mode with the driver_version */
  if ((this->nd_gpusn[gpuidx].driver_ver == 260) &&
      (gpumode > 2))
    {
    err_msg = "GPU driver version does not support mode 3";
    return(false);
    }

  return(true);
  }



enum psit pbsnode::get_flag() const

  {
  return(this->nd_flag);
  }



pbsnode::~pbsnode()

  {
  u_long          *up;

  remove_node(&allnodes, this);
  this->unlock_node(__func__, NULL, LOGLEVEL);
  free(this->nd_mutex);

  if (this->nd_micjobs != NULL)
    free(this->nd_micjobs);

  free_prop_list(this->nd_first);

  this->nd_first = NULL;

  if (this->nd_addrs != NULL)
    {
    for (up = this->nd_addrs;*up != 0;up++)
      {
      /* del node's IP addresses from tree  */

      ipaddrs = AVL_delete_node(*up, this->get_service_port(), ipaddrs);
      }

    if (this->nd_addrs != NULL)
      {
      /* remove array of IP addresses */

      free(this->nd_addrs);

      this->nd_addrs = NULL;
      }
    }

  if (this->nd_gpusn != NULL)
    free(this->nd_gpusn);

  if (this->alps_subnodes != NULL)
    delete this->alps_subnodes;

  if (this->nd_ms_jobs != NULL)
    delete this->nd_ms_jobs;

  if (this->nd_acl != NULL)
    {
    if (this->nd_acl->as_buf != NULL)
      {
      free(this->nd_acl->as_buf);
      }
    free(this->nd_acl);
    }

  if (this->nd_requestid != NULL)
    delete this->nd_requestid;

  if (this->nd_name != NULL)
    delete this->nd_name;

  if (this->nd_error != NULL)
    delete this->nd_error;
  } // END destructor


void pbsnode::set_name(

  const char *name)

  {
  if (this->nd_name != NULL)
    delete this->nd_name;

  this->nd_name = new std::string(name);
  }



void pbsnode::set_node_id(

  int nid)

  {
  this->nd_id = nid;
  }

