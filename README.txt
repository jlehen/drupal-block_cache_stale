README.txt
==========

Block Cache Stale is an answer to clear cache invalidation and regeneration.
It aims to — asynchronously — regenerate block and page cache via a CLI daemon in order to your visitor and contributor do not have to suffer the time taken to recreate it.

The principle is to:
- catch events that normally clear cache, 
- pass it to a CLI drush daemon via ØMQ.
- use a HEAD http request on the page with some parameters (block or page to invalidate)
- this request is treated by a new context block plugin that call a kind-of-copy of _block_render_block (from core block module) to 

It relies on context, (because you should not make drupal sites without, and this module is specially made for website using contect). But context dooesn't permit by default to use block cache, so you have to add blockcache_alter and context_blockcache_alter. The use of blockcache_alter permits to configure block per block the cache configuration between a lots of rules.



-- REQUIREMENTS --

Server side:
- zmq (http://www.zeromq.org/)
- PHP PECL Extention zmq: ømq (pear channel-discover pear.zero.mq && pecl install zero.mq/zmq-beta)
- a daemon-izer like supervisor (http://supervisord.org/)

PHP Code side:
- guzzle, installed with composer

Drupal side:
- blockcache_alter
- context_blockcache_alter
- contextual
- expire


Installation
------------

{{{
set the $base_url value in settings.php 

drush dl composer
cd sites/all/modules/contrib/blockcache_stale
drush composer install

drush en blockcache_stale blockcache_stale_contextual blockcache_stale_expire

# launch daemon
drush blockcache_stale_daemon
}}}

#TODO: explain how to configure with supervisor
(cf. conf http://glitterbug.in/blog/task-queuing-in-django-with-zeromq-5/show/)

Configuration 
-------------

Go here: admin/config/development/performance/expire
and disable "Include base URL in expires"

By default, the ØMQ daemon listens on tcp://*:1234 and the Drupal module
connects to tcp://localhost:1234.

You can change the URL with which you can reach the daemon in settings.php:
$conf['blockcache_stale_zmq_connect_url'] = 'tcp://otherhost:1411'

Likewise, you can configure the bind URL for the daemon:
$conf['blockcache_stale_zmq_daemon_bind_url'] = 'tcp://otherhost:1411'

If for some reason you want to run multiple daemons from the same Drupal
site configuration and bind them to different URLs, you can derive the
BcsMasterZmq class.  The URL can be provided as the first parameter of the
constructor, it overrides the configuration from settings.php.

Different ØMQ URL styles are described here:
http://api.zeromq.org/2-2:zmq-connect

Difficulties solved
-------------------

- do not integrate $_GET parameters in block cache key


Limitation
----------

- cache_clear_all() on node_form_submit # patch it?
- head request not yet parallel (https://github.com/guzzle/guzzle)
- we should handle gracefully block that are in global cache



Author / Maintainers
--------------------
- bastnic (Bastien Jaillot) <bastien at jaillot DOT org>
