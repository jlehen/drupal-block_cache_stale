<?php
require_once(drupal_get_path('module', 'context_blockcache_alter') . '/plugins/context_blockcache_alter_reaction_block.inc');

class blockcache_stale_reaction_block extends context_blockcache_alter_reaction_block {

  /**
   * An alternative version of block_list() that provides any context enabled blocks.
   * **UPDATED** by bjaillot for tests on a stale cache implementatation
   */
  function block_list($region) {
    module_load_include('module', 'block', 'block');

    $context_blocks = &drupal_static('context_reaction_block_list');;
    $contexts = context_active_contexts();
    if (!isset($context_blocks)) {
      $info = $this->get_blocks();
      $context_blocks = array();
      foreach ($contexts as $context) {
        $options = $this->fetch_from_context($context);
        if (!empty($options['blocks'])) {
          foreach ($options['blocks'] as $block) {
            $block = (object) $block;
            $block->context = $context->name;
            $block->bid = "{$block->module}-{$block->delta}";
            $block->title = isset($info[$block->bid]->title) ? $info[$block->bid]->title : NULL;
            $block->cache = isset($info[$block->bid]->cache) ? $info[$block->bid]->cache : DRUPAL_NO_CACHE;
            $context_blocks[$block->region][$block->bid] = $block;
          }
        }
      }

      $this->is_editable_check($context_blocks);
      foreach ($context_blocks as $r => $blocks) {
        $context_blocks[$r] = blockcache_stale_block_render_blocks($blocks);

        // Make blocks editable if allowed.
        if ($this->is_editable_region($r)) {
          foreach ($context_blocks[$r] as $key => $block) {
            $context_blocks[$r][$key] = $this->editable_block($block);
          }
        }

        // Sort blocks.
        uasort($context_blocks[$r], array('context_reaction_block', 'block_sort'));
      }
    }
    return isset($context_blocks[$region]) ? $context_blocks[$region] : array();
  }

  /**
   * Override of block_list() to add blockcache_alter functionality.
   * Use this patch from @Fabianx
   * https://drupal.org/files/0001-Issue-1251240-Use-block-cache-setting-from-DB.patch
   */
  function get_blocks($region = NULL, $context = NULL, $reset = FALSE) {
    static $block_info;

    if (!isset($block_info) || $reset) {
      $block_info = array();
      if (!$reset) {
        $block_info = context_cache_get('block_info');
      }
      if (empty($block_info)) {
        $block_info = array();
        foreach (module_implements('block_info') as $module) {
          $module_blocks = module_invoke($module, 'block_info');
          if (!empty($module_blocks)) {
            foreach ($module_blocks as $delta => $block) {
              $block = (object) $block;
              $block->module = $module;
              $block->delta = $delta;
              $block->bid = "{$block->module}-{$block->delta}";
              $block_info[$block->bid] = $block;
            }
          }
        }
        context_cache_set('block_info', $block_info);
      }
      // Allow other modules that may declare blocks dynamically to alter
      // this list.
      drupal_alter('context_block_info', $block_info);

      // Gather only region info from the database.
      if (module_exists('block')) {
        $theme_key = variable_get('theme_default', 'garland');
        $result = db_select('block')
          ->fields('block', array('module', 'weight', 'delta', 'region', 'title', 'cache'))
          ->condition('theme', $theme_key)
          ->execute();
        foreach ($result as $row) {
          if (isset($block_info["{$row->module}-{$row->delta}"])) {
            $block_info["{$row->module}-{$row->delta}"]->weight = $row->weight;
            $block_info["{$row->module}-{$row->delta}"]->region = $row->region;
            $block_info["{$row->module}-{$row->delta}"]->title = $row->title;
            $block_info["{$row->module}-{$row->delta}"]->cache = $row->cache;
          }
        }
      }
    }

    $blocks = array();

    // No region specified, provide all blocks.
    if (!isset($region)) {
      $blocks = $block_info;
    }
    // Region specified.
    else {
      foreach ($block_info as $bid => $block) {
        if (isset($block->region) && $block->region == $region) {
          $blocks[$bid] = $block;
        }
      }
    }

    // Add context blocks if provided.
    if (is_object($context) && $options = $this->fetch_from_context($context)) {
      if (!empty($options['blocks'])) {
        foreach ($options['blocks'] as $block) {
          if (
            isset($block_info["{$block['module']}-{$block['delta']}"]) && // Block is valid.
            (!isset($region) || (!empty($region) && $block['region'] == $region)) // No region specified or regions match.
          ) {
            $context_block = $block_info["{$block['module']}-{$block['delta']}"];
            $context_block->weight = $block['weight'];
            $context_block->region = $block['region'];
            $context_block->context = !empty($context->name) ? $context->name : 'tempname';
            $blocks[$context_block->bid] = $context_block;
          }
        }
      }
      uasort($blocks, array('context_reaction_block', 'block_sort'));
    }
    return $blocks;
  }

}

