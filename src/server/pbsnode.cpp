#include <stdlib.h>
#include <stdio.h>

#include "pbs_nodes.h"
#include "net_cache.h"
#include "id_map.hpp"
#include "../lib/Libnet/lib_net.h"
#include "log.h"
#include "timer.hpp"
#include "pbs_job.h"

void populate_range_string_from_slot_tracker(const execution_slot_tracker &est, std::string &range_str);
int  unlock_ji_mutex(job *pjob, const char *id, const char *msg, int logging);


void pbsnode::set_name(

  char *name)

  {
  if (this->nd_name != NULL)
    free(this->nd_name);

  this->nd_name = name;
  }



pbsnode::pbsnode(

  char           *pname, /* node name */
  u_long         *pul,  /* host byte order array of ipaddrs for this node */
  bool            isNUMANode) : nd_first(NULL), nd_last(NULL), nd_f_st(NULL), nd_l_st(NULL),
                                nd_addrs(NULL), nd_prop(NULL), nd_status(NULL), nd_note(NULL),
                                nd_stream(0), nd_flag(okay), nd_mom_port(0), nd_mom_rm_port(0),
                                nd_sock_addr(), nd_nprops(0), nd_slots(), nd_job_usages(),
                                nd_needed(0), nd_np_to_be_used(0), nd_state(0), nd_ntype(0),
                                nd_order(0), nd_warnbad(0), nd_lastupdate(0), nd_hierarchy_level(0),
                                nd_in_hierarchy(0), nd_ngpus(0), nd_gpus_real(0), nd_gpusn(NULL),
                                nd_ngpus_free(0), nd_ngpus_needed(0), nd_ngpus_to_be_used(0),
                                nd_gpustatus(NULL), nd_ngpustatus(0), nd_nmics(0),
                                nd_micstatus(NULL), nd_micjobs(NULL), nd_nmics_alloced(0),
                                nd_nmics_free(0), nd_nmics_to_be_used(0), parent(NULL),
                                num_node_boards(0), node_boards(NULL), numa_str(NULL),
                                gpu_str(NULL), nd_mom_reported_down(0), nd_is_alps_reporter(0),
                                nd_is_alps_login(0), nd_ms_jobs(NULL), alps_subnodes(NULL),
                                max_subnode_nppn(0), nd_power_state(0),
                                nd_power_state_change_time(0), nd_acl(NULL), nd_requestid(NULL),
                                nd_error()

  {
  struct addrinfo *pAddrInfo;

  this->nd_name = pname;
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
  if (!isNUMANode)
    {
    if (pbs_getaddrinfo(pname,NULL,&pAddrInfo))
      {
      nd_error = "Couldn't resolve hostname '";
      nd_error += pname;
      nd_error += "'.";
      return;
      }

    memcpy(&this->nd_sock_addr,pAddrInfo->ai_addr,sizeof(struct sockaddr_in));
    }

  this->nd_mutex = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
  if (this->nd_mutex == NULL)
    {
    nd_error = "Could not allocate memory for the node's mutex";
    log_err(ENOMEM, __func__, "Could not allocate memory for the node's mutex");
    }
  else
    pthread_mutex_init(this->nd_mutex,NULL);
  } // END pbsnode(char *, unsigned long *, bool)



pbsnode::pbsnode() : nd_first(NULL), nd_last(NULL), nd_f_st(NULL), nd_l_st(NULL), nd_addrs(NULL),
                     nd_prop(NULL), nd_status(NULL), nd_note(NULL), nd_stream(0), nd_flag(okay),
                     nd_mom_port(0), nd_mom_rm_port(0), nd_sock_addr(), nd_nprops(0), nd_slots(),
                     nd_job_usages(), nd_needed(0), nd_np_to_be_used(0), nd_state(0), nd_ntype(0),
                     nd_order(0), nd_warnbad(0), nd_lastupdate(0), nd_hierarchy_level(0),
                     nd_in_hierarchy(0), nd_ngpus(0), nd_gpus_real(0), nd_gpusn(NULL),
                     nd_ngpus_free(0), nd_ngpus_needed(0), nd_ngpus_to_be_used(0),
                     nd_gpustatus(NULL), nd_ngpustatus(0), nd_nmics(0), nd_micstatus(NULL),
                     nd_micjobs(NULL), nd_nmics_alloced(0), nd_nmics_free(0),
                     nd_nmics_to_be_used(0), parent(NULL), num_node_boards(0), node_boards(NULL),
                     numa_str(NULL), gpu_str(NULL), nd_mom_reported_down(0),
                     nd_is_alps_reporter(0), nd_is_alps_login(0), nd_ms_jobs(NULL),
                     alps_subnodes(NULL), max_subnode_nppn(0), nd_power_state(0),
                     nd_power_state_change_time(0), nd_acl(NULL), nd_requestid(NULL),
                     nd_mutex(NULL), nd_error()

  {
  } // END pbsnode()



pbsnode::pbsnode(
    
  const pbsnode &other) : nd_first(other.nd_first), nd_last(other.nd_last),
                     nd_f_st(other.nd_f_st), nd_l_st(other.nd_l_st), nd_addrs(other.nd_addrs),
                     nd_prop(other.nd_prop), nd_status(other.nd_status), nd_note(other.nd_note),
                     nd_stream(other.nd_stream), nd_flag(other.nd_flag),
                     nd_mom_port(other.nd_mom_port), nd_mom_rm_port(other.nd_mom_rm_port),
                     nd_sock_addr(other.nd_sock_addr), nd_nprops(other.nd_nprops),
                     nd_slots(other.nd_slots), nd_job_usages(other.nd_job_usages),
                     nd_needed(other.nd_needed), nd_np_to_be_used(other.nd_np_to_be_used),
                     nd_state(other.nd_state), nd_ntype(other.nd_ntype), nd_order(other.nd_order),
                     nd_warnbad(other.nd_warnbad), nd_lastupdate(other.nd_lastupdate),
                     nd_hierarchy_level(other.nd_hierarchy_level),
                     nd_in_hierarchy(other.nd_in_hierarchy), nd_ngpus(other.nd_ngpus),
                     nd_gpus_real(other.nd_gpus_real), nd_gpusn(other.nd_gpusn),
                     nd_ngpus_free(other.nd_ngpus_free), nd_ngpus_needed(other.nd_ngpus_needed),
                     nd_ngpus_to_be_used(other.nd_ngpus_to_be_used),
                     nd_gpustatus(other.nd_gpustatus), nd_ngpustatus(other.nd_ngpustatus),
                     nd_nmics(other.nd_nmics), nd_micjobs(other.nd_micjobs),
                     nd_micstatus(other.nd_micstatus), nd_nmics_alloced(other.nd_nmics_alloced),
                     nd_nmics_free(other.nd_nmics_free),
                     nd_nmics_to_be_used(other.nd_nmics_to_be_used), parent(other.parent),
                     num_node_boards(other.num_node_boards), node_boards(other.node_boards),
                     numa_str(other.numa_str), gpu_str(other.gpu_str),
                     nd_mom_reported_down(other.nd_mom_reported_down),
                     nd_is_alps_reporter(other.nd_is_alps_reporter),
                     nd_is_alps_login(other.nd_is_alps_login), nd_ms_jobs(other.nd_ms_jobs),
                     alps_subnodes(other.alps_subnodes), max_subnode_nppn(other.max_subnode_nppn),
                     nd_power_state(other.nd_power_state),
                     nd_power_state_change_time(other.nd_power_state_change_time),
                     nd_acl(other.nd_acl), nd_requestid(other.nd_requestid),
                     nd_mutex(other.nd_mutex), nd_error()

  {
  memcpy(this->nd_mac_addr, other.nd_mac_addr, sizeof(this->nd_mac_addr));
  memcpy(this->nd_ttl, other.nd_ttl, sizeof(this->nd_ttl));
  }



int pbsnode::get_node_id() const

  {
  return(this->nd_id);
  }



const char *pbsnode::get_name() const

  {
  return(this->nd_name);
  } // END get_node_name()



const char *pbsnode::get_error() const

  {
  return(this->nd_error.c_str());
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
