<?php

/**
 * @file
 * Drush integration for the blockcache_stale module.
 *
 * This script ensure that only one import is done at a time
 * It uses:
 * - zeromq socket to communicate with a webservice.
 * - …
 *
 * Launch it as a service with:
 * $ cd DRUPAL_ROOT && drush blockcache-stale-dameon
 *
 */

/**
 * Implements hook_drush_command().
 */
function blockcache_stale_drush_command() {

  $items['blockcache-stale-dameon'] = array(
    'description' => dt('Launch zmq daemon'),
    'arguments' => array(
    ),
    'aliases' => array(),
  );

  return $items;
}

/**
 * A command callback.
 */
function drush_blockcache_stale_dameon() {
  echo "listening\n";

  $zmq = new BcsMasterZmq();
  $zmq->listen();
}

class BcsMasterZmq {

  // zmq sockets
  protected $pub = null;
  protected $pull = null;

  // master site directory
  private $master_dir = '';

  // log file
  private $log_file = '';

  /*
   * create the ZMQ zockets
   */
  public function __construct($pull_port = 1234, $pub_port = 5560) {

    // pull receiver
    $ctx = new ZMQContext();
    $this->pull = new ZMQSocket($ctx, ZMQ::SOCKET_PULL);
    $this->pull->connect('tcp://localhost:' . $pull_port);

    // pub-sub publisher
    // not useful yet because we don't have workers
    // $this->pub = new ZMQSocket($ctx, ZMQ::SOCKET_PUB);
    // $this->pub->bind("tcp://*:" . $pub_port);
  }

  public function listen() {

    // treatment
    while (true) {
      // on data reception
      $params = $this->pull->recvmulti();

      $msg = array_shift($params);

      $this->notifyRunning($msg);

      // handle suicide
      if ($msg == 'KILL') {
        exit(0);
      }

      // generate action name
      // form:drupal/msg:clear-blocks => actionClearBlocks
      // form:drupal/msg:clear-pages => actionClearPages
      $action = preg_replace('#from:(?:.*)/msg:(.*)$#', '$1', $msg);
      $action = 'action' . str_replace(' ', '', ucwords(str_replace('-', ' ', $action)));

      // call the action method
      call_user_func_array(array(&$this, $action), $params);

      // import is done
      $this->notifyRunning();
    }

  }

  protected function actionClearBlocks($params = array()) {
    blockcache_stale_regenerate_blocks(unserialize($params));
  }

  protected function actionClearPages($params = array()) {
    blockcache_stale_regenerate_page(unserialize($params));
  }

  private function notifyRunning($task = 'none') {
    $msg = sprintf('— %s — since %s', $task, format_date(time()));
    variable_set('bcs_zmq_running_task', $msg);
    print $msg . "\n";
  }

}

