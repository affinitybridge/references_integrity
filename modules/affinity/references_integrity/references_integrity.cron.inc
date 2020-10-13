<?php

/**
 * @file
 *   Queue consumer.
 */

/**
 * Queue fetcher.
 */
function references_integrity_fetch() {
  $queue = DrupalQueue::get('references_integrity');

  $count = 0;
  $references_integrity_cron_records_per_run = variable_get('references_integrity_cron_records_per_run', 50);

  while ($item = $queue->claimItem()) {

    // Load the entity.
    $entity_type = $item->data['entity_type'];
    $entity_id = $item->data['entity_id'];
    $field_name = $item->data['field_name'];
    $column = $item->data['column'];
    $entity = entity_load_single($entity_type, $entity_id);
    $op = $item->data['op'];

    if ($entity) {
      if ($op == REFERENCES_INTEGRITY_BEHAVIOR_REMOVE_REFERENCE) {
        // Iterate deeply to find the reference.
        foreach ($entity->{$field_name} as $language_code => $delta_group) {
          foreach ($delta_group as $delta => $delta_item) {
            if (in_array($delta_item[$column], $item->data['references'])) {
              unset($entity->{$field_name}[$language_code][$delta]);
            }
          }
        }
        entity_save($entity_type, $entity);
        watchdog('references_integrity', 'Orphan references: @orphans removed from field "@field_name", entity "@entity_type", @id = @entity_id', array('@orphans' => implode(', ', $item->data['references']), '@field_name' => $field_name, '@entity_type' => $entity_type, '@id' => $column, '@entity_id' => $entity_id), WATCHDOG_INFO);
      } elseif ($op == REFERENCES_INTEGRITY_BEHAVIOR_REMOVE_ENTITY) {
        entity_delete($entity_type, $entity_id);
        watchdog('references_integrity', 'Orphan references: @entity_type @entity_id deleted', array('@entity_type' => $entity_type, '@entity_id' => $entity_id), WATCHDOG_INFO);
      }

      $count++;
      $queue->deleteItem($item);

      if ($count > $references_integrity_cron_records_per_run) {
        break;
      }
    }
  }
}
