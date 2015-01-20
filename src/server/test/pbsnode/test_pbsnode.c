#include <stdio.h>
#include <stdlib.h>

#include "pbs_nodes.h"
#include <check.h>

extern int id;


START_TEST(test_constructors)
  {
  id = 0;
  pbsnode pnode("napali", NULL, true);

  fail_unless(!strcmp(pnode.get_name(), "napali"));
  fail_unless(strlen(pnode.get_error()) == 0);
  fail_unless(pnode.get_node_id() == 0);
  fail_unless(pnode.get_mic_count() == 0);
  fail_unless(pnode.gpu_count(true, 0) == 0);
  fail_unless(pnode.get_execution_slot_count() == 0);
  fail_unless(pnode.get_execution_slot_free_count() == 0);
  fail_unless(pnode.get_service_port() == PBS_MOM_SERVICE_PORT);
  fail_unless(pnode.get_manager_port() == PBS_MANAGER_SERVICE_PORT);
  fail_unless(pnode.get_parent() == NULL);

  pbsnode copy(pnode);
  fail_unless(!strcmp(copy.get_name(), "napali"));
  fail_unless(strlen(copy.get_error()) == 0);
  fail_unless(copy.get_node_id() == 0);
  fail_unless(copy.get_mic_count() == 0);
  fail_unless(copy.gpu_count(true, 0) == 0);
  fail_unless(copy.get_execution_slot_count() == 0);
  fail_unless(copy.get_execution_slot_free_count() == 0);
  fail_unless(copy.get_service_port() == PBS_MOM_SERVICE_PORT);
  fail_unless(copy.get_manager_port() == PBS_MANAGER_SERVICE_PORT);
  fail_unless(copy.get_parent() == NULL);

  pnode.set_node_id(2);
  fail_unless(pnode.get_node_id() == 2);
  pnode.set_service_port(1);
  pnode.set_manager_port(3);
  fail_unless(pnode.get_service_port() == 1);
  fail_unless(pnode.get_manager_port() == 3);

  pnode.set_gpu_count(5);
  fail_unless(pnode.gpu_count() == 5);

  // make sure we do nothing if there are no more nodes to delete
  for (int i = 0; i < 10; i++)
    pnode.delete_a_gpusubnode();
  fail_unless(pnode.gpu_count() == 0);

  for (int i = 0; i < 10; i++)
    pnode.add_execution_slot();

  fail_unless(pnode.get_execution_slot_count() == 10);
  fail_unless(pnode.get_execution_slot_free_count() == 10);
  
  }
END_TEST

START_TEST(check_node_for_job_test)
  {
  job_usage_info jui(1);
  struct pbsnode *pnode = new pbsnode();

  for (int i = 0; i < 10; i++)
    pnode->add_execution_slot();

  pnode->reserve_execution_slots(6, jui.est);
  pnode->nd_job_usages.push_back(jui);

  fail_unless(pnode->check_node_for_job(1) == true);
  fail_unless(pnode->is_job_on_node(1) == true);

  pbsnode n;
  
  for (int i = 0; i < 10; i++)
    n.add_execution_slot();

  n.reserve_execution_slots(6, jui.est);
  n.nd_job_usages.push_back(jui);

  }
END_TEST


START_TEST(remove_job_from_node_test)
  {
  job_usage_info jui(1);
  struct pbsnode *pnode = new pbsnode();

  pnode->lock_node(__func__, NULL, 0);

  for (int i = 0; i < 10; i++)
    pnode->add_execution_slot();

  pnode->reserve_execution_slots(6, jui.est);
  pnode->nd_job_usages.push_back(jui);

  fail_unless(pnode->get_execution_slot_free_count() == 4);

  pnode->remove_job_from_node(1);
  fail_unless(pnode->get_execution_slot_free_count() == 10, "count=%d", pnode->get_execution_slot_free_count());
  pnode->remove_job_from_node(1);
  fail_unless(pnode->get_execution_slot_free_count() == 10);

  pnode->unlock_node(__func__, NULL, 0);
  }
END_TEST


START_TEST(node_is_spec_acceptable_test)
  {
  struct pbsnode   pnode;
  single_spec_data spec;
  int              eligible_nodes = 0;

  memset(&spec, 0, sizeof(spec));

  spec.ppn = 10;

  fail_unless(pnode.node_is_spec_acceptable(&spec, NULL, &eligible_nodes, false, 0) == false);
  fail_unless(eligible_nodes == 0);

  for (int i = 0; i < 10; i++)
    pnode.add_execution_slot();
    
  pnode.mark_slot_as_used(4);

  fail_unless(pnode.node_is_spec_acceptable(&spec, NULL, &eligible_nodes,false, 0) == false);
  fail_unless(eligible_nodes == 1);

  eligible_nodes = 0;
  pnode.mark_slot_as_free(4);
  pnode.nd_state |= INUSE_DOWN;  
  fail_unless(pnode.node_is_spec_acceptable(&spec, NULL, &eligible_nodes,false, 0) == false);
  fail_unless(eligible_nodes == 1);
  
  eligible_nodes = 0;
  pnode.nd_state = INUSE_FREE;
  fail_unless(pnode.node_is_spec_acceptable(&spec, NULL, &eligible_nodes,false, 0) == true);
  fail_unless(eligible_nodes == 1);
  }
END_TEST


START_TEST(set_execution_slot_test)
  {
  pbsnode pnode;

  pnode.set_execution_slot_count(5);
  fail_unless(pnode.get_execution_slot_count() == 5);
  pnode.set_execution_slot_count(3);
  fail_unless(pnode.get_execution_slot_count() == 3);
  pnode.set_execution_slot_count(88);
  fail_unless(pnode.get_execution_slot_count() == 88);

  fail_unless(pnode.set_execution_slot_count(-1) == PBSE_BADATVAL);
  fail_unless(pnode.get_execution_slot_count() == 88);
  }
END_TEST


START_TEST(create_a_gpusubnode_test)
  {
  int result = -1;
  struct pbsnode node;

  result = node.create_a_gpusubnode();
  fail_unless(result == PBSE_NONE, "create_a_gpusubnode fail");
  fail_unless(node.gpu_count() == 1);
  }
END_TEST


START_TEST(add_execution_slot_test)
  {
  struct pbsnode node;
  int result = 0;

  result = node.add_execution_slot();
  fail_unless(result == PBSE_NONE, "add_execution_slot_test fail");
  fail_unless(node.get_execution_slot_count() == 1);
  fail_unless(node.is_slot_occupied(0) == false);
  }
END_TEST


START_TEST(login_encode_jobs_test)
  {
  struct pbsnode   node;
  struct list_link list;
  int              result;
  memset(&list, 0, sizeof(list));

  result = node.login_encode_jobs(NULL);
  fail_unless(result != PBSE_NONE, "NULL input list_link pointer fail");

  result = node.login_encode_jobs(&list);
  fail_unless(result != PBSE_NONE, "login_encode_jobs_test fail");
  }
END_TEST



Suite *pbsnode_suite(void)
  {
  Suite *s = suite_create("pbsnode test suite methods");
  TCase *tc_core = tcase_create("test_one");
  tcase_add_test(tc_core, test_constructors);
  tcase_add_test(tc_core, remove_job_from_node_test);
  tcase_add_test(tc_core, login_encode_jobs_test);
  suite_add_tcase(s, tc_core);
  
  tc_core = tcase_create("test_two");
  tcase_add_test(tc_core, node_is_spec_acceptable_test);
  tcase_add_test(tc_core, create_a_gpusubnode_test);
  tcase_add_test(tc_core, add_execution_slot_test);
  tcase_add_test(tc_core, set_execution_slot_test);
  tcase_add_test(tc_core, check_node_for_job_test);
  suite_add_tcase(s, tc_core);
  
  return(s);
  }

void rundebug()
  {
  }

int main(void)
  {
  int number_failed = 0;
  SRunner *sr = NULL;
  rundebug();
  sr = srunner_create(pbsnode_suite());
  srunner_set_log(sr, "pbsnode_suite.log");
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return(number_failed);
  }
