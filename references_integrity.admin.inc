<?php

/**
 * @file
 * Implements administration pages of the module.
 */

/**
 * Alter the field settings form for reference fields.
 */
function _references_integrity_field_settings_form_alter(&$form, $form_state, $form_id) {
  $field_name = $form['field']['field_name']['#value'];
  if ($form['field']['type']['#value'] == 'node_reference'){
    $field_entities = t('nodes');
  } elseif($form['field']['type']['#value'] == 'user_reference'){
    $field_entities = t('users');

  } elseif ($form['field']['type']['#value'] == 'entityreference'){
    $field_entities = t('entities');
  }

  $form['field']['references_integrity_behavior'] = array(
    '#type' => 'radios',
    '#title' => t('Referential integrity behavior'),
    '#options' => references_integrity_get_behavior_options($field_entities),
    '#default_value' => variable_get('references_integrity_behavior_' . $field_name, ''),
    '#description' => t('This option defines the referential integrity behavior that will be enforced upon deletion of @entities referenced from this field.', array('@entities' => $field_entities)),
  );
  $form['field']['references_integrity_delete_message'] = array(
    '#type' => 'textfield',
    '#title' => t('Custom delete message'),
    '#default_value' => variable_get('references_integrity_delete_message_' . $field_name, ''),
    '#description' => t('Warn your user that referential integrity behavior will be enforced upon deletion of @entities referenced from this field.', array('@entities' => $field_entities)),
    '#states' => array(
      'visible' => array(
        ':input[name="field[references_integrity_behavior]"]' => array(array('value' => 'remove reference'), array('value' => 'remove entity')),
      ),
    ),
  );
  $form['#submit'][] = '_references_integrity_field_settings_form_alter_submit';
}

/**
 * Build the orphan references report for all fields.
 */
function references_integrity_orphan_references_check_all($reference_type = 'all') {
  // Include common module functions.
  module_load_include('inc', 'references_integrity');

  $field_status_titles = array(
    'ok' => t('No orphan records have been found for this field.'),
    'warning' => t('Orphan records have been found for this field.'),
  );

  // Load information about all reference fields in the system.
  $reference_fields = array();
  foreach (array('user_reference', 'node_reference', 'entityreference') as $field_type) {
    if ($field_type == $reference_type || $reference_type == 'all'){
      $reference_fields = array_merge($reference_fields, references_integrity_get_reference_fields($field_type));
    }
  }

  // Render the report.
  drupal_add_css(drupal_get_path('module', 'references_integrity') . '/css/admin.css', 'module', 'all', FALSE);
  $headers = array(
    array('data' => t('Referencing Entity')),
    array('data' => t('Bundle')),
    array('data' => t('Field')),
    array('data' => t('Type')),
    array('data' => t('Records'), 'class' => 'references-integrity-cell-numeric references-integrity-record-count'),
    array('data' => t('Orphans'), 'class' => 'references-integrity-cell-numeric references-integrity-orphan-count'),
    array('data' => t('Status'), 'class' => 'references-integrity-cell-status'),
  );

  $rows = array();
  $fields_with_orphan_records = FALSE;
  foreach ($reference_fields as $delta => $field_info) {
    if (! $field_info['entity_name']){
      continue;
    }
    $row = array();
    $status = theme('references_integrity_status_icon', array('status' => $field_info['status'], 'title' => $field_status_titles[$field_info['status']]));
    $field = t($field_info['label']) .' ['. $field_info['field_name'] .']';
    if ($field_info['status'] != 'ok') {
      $fields_with_orphan_records = TRUE;
      $table_cell = l($field, 'admin/content/orphan-references/'. $field_info['entity_type'] . '/' . $field_info['bundle'] . '/' . $field_info['field_name'] . '/' . $field_info['field_type']);
    }
    else {
      $table_cell = $field;
    }
    $row[] = array('data' => $field_info['entity_name'] . ' [' . $field_info['entity_type'] . ']');
    $row[] = array('data' => $field_info['bundle_name'] . ' [' . $field_info['bundle'] . ']');
    $row[] = array('data' => $table_cell, 'title' => $field);
    $row[] = array('data' => $field_info['field_type']);
    $row[] = array('data' => $field_info['count'], 'class' => 'references-integrity-cell-numeric references-integrity-record-count');
    $row[] = array('data' => $field_info['orphan'], 'class' => 'references-integrity-cell-numeric references-integrity-orphan-count');
    $row[] = array('data' => $status, 'class' => 'references-integrity-cell-status');
    $class = array('class' => $field_info['entity_type'] . '-' . $field_info['bundle'] . '-' . $field_info['field_name']);

    $rows[] = array('data' => $row, 'class' => $class);
  }

  $rows_count = count($rows);
  if ($rows_count <= 0) {
    $rows[] = array(array('colspan' => count($headers), 'align' => 'center', 'data' => t('Could not find reference fields.')));
  }

  $help = t('This report displays information about the node and user reference fields occurences.
The <em>Records</em> column is the total number of records stored for each field. The <em>Orphans</em> column is the number of records that refer to non-existing nodes or users respectively.
A warning icon is displayed on the status column when orphans are found for a particular instance, and you can then click on the field name to review the corresponding orphan records.');
  $output = '<div class="help">'. $help .'</div>'."\n";
  $output .= theme('table', array('header' => $headers, 'rows' => $rows));
  if ($fields_with_orphan_records) {
    $warning = t('Orphan records have been found for some fields. You may now click on the field name to review the corresponding orphan records.');
    $output .= '<div class="messages warning"><span class="warning">*</span> '. $warning .'</div>';
  }
  return $output;
}

/**
 * Page callback for detailed field reference check.
 *
 * Build the orphan references report for just one field.
 */
function references_integrity_orphan_references_check_field($form, &$form_state, $entity_type, $bundle, $field, $field_type) {

  // Since we just need the entity type, just use it.
  $entity_type = $entity_type['entity_type'];

    drupal_set_breadcrumb(array(
      l(t('Home'), '<front>'),
      l(t('Administration'), 'admin'),
      l(t('Content'), 'admin/content'),
      l(t('Orphan references'), 'admin/content/orphan-references'),
    ));

  // Include common module functions.
  module_load_include('inc', 'references_integrity');
  $id = references_integrity_get_id_key($field_type);
  list($target_entity_type, $target_entity_id) = references_integrity_get_target_entity_type_and_id($field);

  $header = array(
    'entity_id' => array('data' => t('ID')),
    'entity' => array('data' => t('Entity')),
    'reference_id' => array('data' => t('Orphaned @id (@type)', array('@id' => $id, '@type' => $field_type))),
    'language' => array('data' => t('Language')),
    'delta' => array('data' => t('Delta')),
  );
  $options = array();

  $query = new EntityFieldQuery;
  $query->entityCondition('entity_type', $field['entity_type']);
  if ($field['entity_type'] != 'comment'){
    $query->entityCondition('bundle', $field['bundle']);
  }
  $result = $query->execute();

  while (list($type, $entities) = each($result)) {

    list($table, $field_info) = each($field['field_info']['storage']['details']['sql'][FIELD_LOAD_CURRENT]);
    list($column, $columnname) = each($field_info);

    $references = array();
    foreach (entity_load($field['entity_type'], array_keys($entities)) as $entity_id => $entity) {
      $name = $field['field_name'];
      foreach ($entity->{$name} as $language_code => $delta_group) {
        foreach ($delta_group as $delta => $delta_item) {
          $references[] = $delta_item[$column];
          $uri = entity_uri($entity_type, $entity);
          $uri_options = empty($uri['options']) ? array() : $uri['options'];
          $options[$entity_id . ':' . $delta_item[$column]] = array(
            'entity_id' => array('data' => $entity_id),
            'entity' => array('data' => $uri ? l(entity_label($entity_type, $entity), $uri['path'], $uri_options) : $entity_type),
            'reference_id' => $delta_item[$column],
            'language' => ($language_code == LANGUAGE_NONE) ? t('Neutral') : $language_code,
            'delta' => $delta,
            'title' => array('data' => array('#title' => entity_label($entity_type, $entity))),
          );
        }
      }
    }

    $referenced_ids = array_keys(entity_load($target_entity_type, $references));

    // Remove non-orphans
    foreach ($options as $key => $option) {
      if (in_array($option['reference_id'], $referenced_ids)) {
        unset($options[$key]);
      }
    }
  }

  $form['orphans'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
  );

  if (!empty($options)) {
    $form['entity_type'] = array(
      '#type' => 'value',
      '#value' => $field['entity_type'],
    );
    $form['field_name'] = array(
      '#type' => 'value',
      '#value' => $field['field_name'],
    );
    $form['column'] = array(
      '#type' => 'value',
      '#value' => $column,
    );
    $form['operations'] = array(
      '#type' => 'fieldset',
      '#title' => t('Operations'),
    );
    $form['operations']['backups'] = array(
      '#type' => 'checkbox',
      '#title' => t('Yes, I have backups of my database.'),
      '#prefix' => '<div class="references-integrity-operations-backups">',
      '#suffix' => '</div>',
    );
    $form['operations']['remove reference'] = array(
      '#type' => 'submit',
      '#value' => t('Remove orphan references'),
    );
    $form['operations']['remove entity'] = array(
      '#type' => 'submit',
      '#value' => t('Remove orphaned entities'),
    );
  }

  return $form;
}

/**
 * Validate handler for the orphan references report for just one field form.
 */
function references_integrity_orphan_references_check_field_validate($form, &$form_state) {
  if (empty($form_state['values']['backups'])) {
    form_set_error('backups', t('Please, make backups before processing orphan records.'));
  }
  $form_state['values']['orphans'] = array_filter($form_state['values']['orphans']);
  if (count($form_state['values']['orphans']) == 0) {
    form_set_error('orphans', t('No records selected.'));
  }
}

/**
 * Submit handler for the orphan references report for just one field form.
 */
function references_integrity_orphan_references_check_field_submit($form, &$form_state) {
  $values =& $form_state['values'];

  $queue = DrupalQueue::get('references_integrity');
  $queue->createQueue();

  $items = array();
  foreach ($form_state['values']['orphans'] as $orphan) {
    list($entity_id, $id) = explode(':', $orphan);
    $key = $values['entity_type'] . ':' . $entity_id;
    if (!isset($items[$key])) {
      $items[$key] = array(
        'entity_type' => $values['entity_type'],
        'entity_id' => $entity_id,
        'field_name' => $values['field_name'],
        'column' => $values['column'],
        'references' => array(),
        'op' =>  $values['op'] == 'Remove orphaned entities' ?  REFERENCES_INTEGRITY_BEHAVIOR_REMOVE_ENTITY : REFERENCES_INTEGRITY_BEHAVIOR_REMOVE_REFERENCE
      );
    }
    $items[$key]['references'][] = $id;
  }

  foreach ($items as $item) {
    $queue->createItem($item);
  }
  drupal_set_message(t('The selected records have been queued for processing on cron. You can <a target="_blank" href="/admin/reports/status/run-cron">run cron now</a>.'));
}

/**
 * Theme the report status icons.
 *
 * @ingroup themeable
 */
function theme_references_integrity_status_icon($variables = array()) {
  return '<div class="references-integrity-status-'. $variables['status'] .'"'. (!empty($variables['title']) ? ' title="'. check_plain($variables['title']) .'"' : '') .'></div>';
}
