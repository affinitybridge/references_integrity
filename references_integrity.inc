<?php

/**
 * @file
 * Common functions for the References Integrity.
 */

/**
 * Get the list of referential integrity behaviors.
 *
 * @param $field_entities
 *   You can currently use either t('nodes') or t('users').
 */
function references_integrity_get_behavior_options($field_entities) {
  return array(
    REFERENCES_INTEGRITY_BEHAVIOR_NONE             => t('Do nothing.'),
    REFERENCES_INTEGRITY_BEHAVIOR_REMOVE_REFERENCE => t('Delete references to @entities when referenced @entities are deleted.', array('@entities' => $field_entities)),
    REFERENCES_INTEGRITY_BEHAVIOR_REMOVE_ENTITY    => t('Delete this entity when referenced @entities are deleted.', array('@entities' => $field_entities)),
  );
}

/**
 * Get the RI behavior defined for the given field name.
 */
function references_integrity_get_field_behavior($field_name) {
  return variable_get('references_integrity_behavior_' . $field_name, '');
}

/**
 * Get the RI message defined for the given field name.
 */
function references_integrity_get_delete_message($field_name) {
  $op = references_integrity_get_field_behavior($field_name);
  if ($op == REFERENCES_INTEGRITY_BEHAVIOR_REMOVE_REFERENCE){
    $default_message = t('References to this entity will be deleted too.');
  } elseif ($op == REFERENCES_INTEGRITY_BEHAVIOR_REMOVE_ENTITY){
    $default_message = t('Entities referencing this entity will be deleted too.');
  }
  return variable_get('references_integrity_delete_message_' . $field_name, '');
}

/**
 * Get information about reference fields of the given type.
 *
 * @param $field_type
 *   Field types supported are 'node_reference', 'user_reference', and 'entityreference'
 */
function references_integrity_get_reference_fields($field_type, $counters = TRUE) {
  static $entity_info = array();
  $fields = array();

  foreach (field_info_instances() as $entity_type => $entity) {
    if (!isset($entity_info[$entity_type])) {
      $entity_info = entity_get_info($entity_type);
    }
    foreach ($entity as $bundle_name => $bundle) {
      foreach ($bundle as $field_name => $field) {
        $field += field_info_field($field_name);
        $field += array(
          'entity_name' => $entity_info['label'],
          'bundle_name' => isset($entity_info['bundles'][$bundle_name]['label']) ? $entity_info['bundles'][$bundle_name]['label'] : NULL,
        );
        if ($field['type'] == $field_type) {
          $fields[] = references_integrity_get_field_info($field, $counters);
        }
      }
    }
  }

  return $fields;
}

/**
 * Get orphans related information about a single field.
 *
 * @param $field
 *   The field to retrieve information from.
 */
function references_integrity_get_field_info($field, $counters = TRUE) {
  if (empty($field) || empty($field['field_name']) || empty($field['type']) || !in_array($field['type'], references_integrity_references())) {
    return FALSE;
  }
  $counters_array = array('count' => 0, 'orphan' => 0, 'status' => 'ok');

  if ($counters) {

    $id = references_integrity_get_id_key($field['type']);

    $query = new EntityFieldQuery;

    $query->entityCondition('entity_type', $field['entity_type']);
    if ($field['entity_type'] != 'user'){
      $query->entityCondition('bundle', $field['bundle']);
    }
    $query->fieldCondition($field['field_name'], $id, 0, '>');
    $result = $query->execute();

    $total_count = $orphan_count = 0;
    while (list($type, $referring_entities) = each($result)) {

      list($table, $field_info) = each($field['storage']['details']['sql'][FIELD_LOAD_CURRENT]);
      list($column, $columnname) = each($field_info);

      $references = array();
      foreach (entity_load($field['entity_type'], array_keys($referring_entities)) as $entity_id => $entity) {
        $name = $field['field_name'];
        foreach ($entity->{$name} as $language_code => $delta_group) {
          foreach ($delta_group as $delta => $delta_item) {
            $references[] = $delta_item[$column];
          }
        }
      }

      // Total count is the count of references, not of referring entities.
      $total_count = count($references);

      list($target_entity_type, $target_entity_id) = references_integrity_get_target_entity_type_and_id($field);
      $valid_referenced_ids = array();
      // The ids of entities that actually exist
      if (!isset($target_entity_id)) {
        $valid_referenced_ids = array_keys(entity_load($target_entity_type, $references));
      }
      else {
        $query = new EntityFieldQuery();
        $query->entityCondition('entity_type', $target_entity_type)
          ->propertyCondition($target_entity_id, array_unique($references), 'IN');
        $efq_result = $query->execute();
        if (isset($efq_result[$target_entity_type])) {
          $valid_referenced_ids = array_keys($efq_result[$target_entity_type]);
        }
      }

      // dpm('REFERENCES');
      // dpm($references);
      // dpm('REFERENCED IDS');
      // dpm($valid_referenced_ids);

      $orphan_count = 0;
      foreach ($references as $i => $reference) {
        if (!in_array($reference, $valid_referenced_ids)) {
          $orphan_count++;
        }
      }

      $counters_array = array(
        'count' => $total_count,
        'orphan' => $orphan_count,
        'status' => $orphan_count > 0 ? 'warning' : 'ok',
      );
    }
  }

  return array(
    'entity_type' => $field['entity_type'],
    'entity_name' => $field['entity_name'],
    'bundle' => $field['bundle'],
    'bundle_name' => $field['bundle_name'],
    'field_name' => $field['field_name'],
    'field_type' => $field['type'],
    'has_delta' => (!empty($field['multiple']) ? TRUE : FALSE),
    'label' => $field['label'],
    'ri_behavior' => references_integrity_get_field_behavior($field['field_name']),
  ) + $counters_array;
}

/**
 *  function references_integrity_get_target_entity_type_and_id
 *
 *  - Return the foreign key information needed for the EFQ
 *
 *  @param $field
 *
 *  list($target_entity_type, $target_entity_id) = references_integrity_get_target_entity_type_and_id($field);
 */
function references_integrity_get_target_entity_type_and_id($field) {
  $field_info = $field;
  if (isset($field['field_info'])) {
    $field_info = $field['field_info'];
  }
  if (isset($field_info['type'])) {
    switch ($field_info['type']) {
      case 'node_reference':
        return array('node', 'nid');
        break;
      case 'user_reference':
        return array('user', 'uid');
        break;
      case 'entityreference':
        if (isset($field_info['foreign keys'])) {
          foreach ($field_info['foreign keys'] as $target_entity_type => $target_data) {
            switch ($target_entity_type) {
              case 'node':
                return array('node', 'nid');
                break;
              case 'taxonomy_term_data':
                return array('taxonomy_term', 'tid');
                break;
            }
          }
        }
        break;
    }
    if (isset($field_info['foreign keys'])) {
      $message = 'References Integrity could not handle the field type !field_type with foreign keys !foreign_keys';
      $variables = array(
        '!field_type' => $field_info['type'],
        '!foreign_keys' => serialize($field_info['foreign keys']),
      );
      watchdog('references_integrity', $message, $variables);
      drupal_set_message(t($message, $variables));
      // Return node by default in order to not crash the page
      return array('node', 'nid');
    }
  }
  $message = 'References Integrity could not handle the field !field';
  $variables = array(
    '!field' => serialize($field),
  );
  watchdog('references_integrity', $message, $variables);
  drupal_set_message(t($message, $variables));
  // Return node by default in order to not crash the page
  return array('node', 'nid');
}

/**
 * Apply referential integrity rule to the given object id.
 * Called prior to deleting an entity.
 *
 * @param $field_type
 *   Field types supported are 'node_reference', 'user_reference', and 'entityreference'
 * @param $field_value
 *   A node id (nid), user id (uid), or entity_id.
 * @param $apply (boolean)
 *   If false items will not be queued. Used to determined how many items will
 *   be modified.
 */
function references_integrity_apply($field_type, $object, $apply = TRUE) {

  $id = references_integrity_get_id_key($field_type);

  $queue = DrupalQueue::get('references_integrity');
  $queue->createQueue();
  $items = array();

  // Get all references field instances.
  foreach (references_integrity_get_reference_fields($field_type, FALSE) as $field) {

    // Process only if the admin enforced referential integrity.
    if ($field['ri_behavior'] == REFERENCES_INTEGRITY_BEHAVIOR_REMOVE_REFERENCE || $field['ri_behavior'] == REFERENCES_INTEGRITY_BEHAVIOR_REMOVE_ENTITY) {

      if ($field['entity_type'] == 'node' && $id == 'target_id'){
        $object_id = 'nid';
      } else {
        $object_id = $id;
      }

      $query = new EntityFieldQuery;
      $query->entityCondition('entity_type', $field['entity_type']);
      $query->entityCondition('bundle', $field['bundle']);
      $query->fieldCondition($field['field_name'], $id, $object->{$object_id}, '=');
      $result = $query->execute();

      if (isset($result[$field['entity_type']])) {
        foreach (entity_load($field['entity_type'], array_keys($result[$field['entity_type']])) as $entity_id => $entity) {
          $item = array(
            'entity_type' => $field['entity_type'],
            'entity_id' => $entity_id,
            'field_name' => $field['field_name'],
            'column' => $id,
            'references' => array($object->{$object_id}),
            'op' => $field['ri_behavior']
          );

          if ($apply){
            // Queue the item for cron processing.
            $queue->createItem($item);
          } else {
            $items[] = $item;
          }
        }
      }
    }
  }
  if (! $apply){
    return $items;
  }
}
