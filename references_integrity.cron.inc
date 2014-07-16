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

  $count = 1;
  while ($item = $queue->claimItem()) {

    // Load the entity.
    $entity_type = $item->data['entity_type'];
    $entity_id = $item->data['entity_id'];
    $field_name = $item->data['field_name'];
    $column = $item->data['column'];
    $entity = reset(entity_load($entity_type, array($entity_id)));

    // Iterate deeply to find the reference.
    foreach ($entity->{$field_name} as $language_code => $delta_group) {
      foreach ($delta_group as $delta => $delta_item) {
        if (in_array($delta_item[$column], $item->data['references'])) {
          unset($entity->{$field_name}[$language_code][$delta]);
        }
      }
    }

    entity_save($entity_type, $entity);

    watchdog('references', 'Orphan refereneces: @orphans removed from field "@field_name", entity "@entity_type", @id = @entity_id', array('@orphans' => implode(', ', $item->data['references']), '@field_name' => $field_name, '@entity_type' => $entity_type, '@id' => $column, '@entity_id' => $entity_id));

    $count++;
    $queue->deleteItem($item);

    if ($count > REFERENCES_INTEGRITY_CRON) {
      break;
    }
  }
}
