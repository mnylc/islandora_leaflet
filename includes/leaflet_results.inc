<?php

/**
 * @file
 * Class override for map display.
 */

/**
 * Extension of IslandoraSolrResults to create an alternative display type.
 */
class IslandoraSolrResultsLeaflet extends IslandoraSolrResults {

  /**
   * Renders the Solr results with Leaflet Map
   *
   *
   * @see IslandoraSolrResults::displayResults()
   *
   * @param array $solr_results
   *   The processed Solr results from
   *   IslandoraSolrQueryProcessor::islandoraSolrResult.
   *
   * @return string
   *   Rendered Solr results.
   */
 
  public function printResults($solr_results) {
    $solr_results = islandora_solr_prepare_solr_results($solr_results);
    $object_results = $solr_results['response']['objects'];
    $object_results = islandora_solr_prepare_solr_doc($object_results);
    $elements = array();
    $elements['solr_total'] = $solr_results['response']['numFound'];
    $elements['solr_start'] = $solr_results['response']['start'];

    // Return themed search results.
    return theme('islandora_leaflet_solr_map', array('results' => $object_results, 'elements' => $elements));
  }
}
