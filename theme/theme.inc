<?php
/**
* @file
* Theme hooks.
*/

/**
 * Implements template_preprocess_HOOK().
 *
 * @param array $variables
 *   The theme template variables.
 * Idea based on islandora_solr_geo module
 * @see leaflet and solr
 * Map requires a latlong solr field. Using XSLT we combined decimal latitud and longitudre the lat/long coordinates in double.
 * @see solrconfig/latlong.xslt and solrconfig/schema.xml
 */
function template_preprocess_islandora_leaflet_solr_map(array &$variables) {
	$module_path = drupal_get_path('module', 'islandora_leaflet');
	module_load_include('inc', 'islandora_solr_search', 'includes/utilities');
	module_load_include('inc', 'islandora_leaflet', 'includes/utilities');
  module_load_include('inc', 'islandora_leaflet', 'includes/callbacks');
  dpm($variables);
  
  $limit_to_cmodel=$variables['limit_to_cmodel']; //This allows us to exclude objects from map based on CMODEL. 
  //By default it's empty so we plot every related object based on our ontology
  
	drupal_set_title(t('Map display'));
	$pathtopoints=array();
		
	$mappings = array(
		'location' => array(
			'cmodels' => 'islandora:sp_basic_image',
			'latlong_lat' => variable_get('islandora_leaft_latlong_lat', 'mods_subject_cartographics_coordinates_p_0_coordinate'),
			'latlong_long' => variable_get('islandora_leaft_latlong_long', 'mods_subject_cartographics_coordinates_p_1_coordinate'),
			'latlong' => variable_get('islandora_leaft_latlong_point', 'mods_subject_cartographics_coordinates_p'),
		),
    
	);
 
	$results = array();

	$content_model_field = variable_get(
	'islandora_solr_content_model_field', 'RELS_EXT_hasModel_uri_ms'
);
   
$leaflet_query_field = $mappings['location']['latlong_lat'];
$stub="";
	    
$query_processor = new IslandoraSolrQueryProcessor();
$query_processoronlythis= new IslandoraSolrQueryProcessor(); //We cant' reuse, because after the first query que processor gets modified
$query_processor->solrQuery = format_string(
'!biodiversidad_query_field:(!biodiversidad_query)', array(
	'!biodiversidad_query_field' => $leaftlet_query_field,
	'!biodiversidad_query' => $stub.'*',
)
);
$query_processor->solrStart = 0;
$query_processor->solrLimit = 30000;

//limit to 30000 for now, leaflet can manage aprox 50.000 cluster points concurrent.
//Let's add some base restrictions to comply with Solr module permissiones
		
$xacml_restrictions=islandora_leaflet_getxacml_solrquery_restrictions();
		
		
		
$query_processor->solrParams = array(
'fl' => $mappings['location']['latlong_lat'].",".$mappings['location']['latlong_long'].",PID,fgs_label_mt,PID_fullurl,dwc.locality_s,dwc.municipality_s,dwc.county_s,dwc.stateProvince_s,dwc.country_s",
'fq' => format_string(
'!content_model_field:"info:fedora/!content_model"', array(
	'!content_model_field' => $content_model_field,
	'!content_model' => $mappings['location']['cmodel'],
)
),
);
 
if (!empty($xacml_restrictions)) {
$query_processor->solrParams['fq'] = $query_processor->solrParams['fq']." AND ( ".$xacml_restrictions." )";
}
		
$url = parse_url(variable_get('islandora_solr_url', 'localhost:8080/solr'));
$solr = new Apache_Solr_Service($url['host'], $url['port'], $url['path'] . '/');
$solr->setCreateDocuments(FALSE);
try {
  
  $results = $solr->search($query_processor->solrQuery, $query_processor->solrStart, $query_processor->solrLimit, $query_processor->solrParams, 'GET');
  $json = json_decode($results->getRawResponse(), TRUE);
//Also get the info from Solr for the current object, fl and fq are the same.
  
 $stubonlythis=urlencode($pid);
 $query_processor->solrQuery = format_string(
    '!biodiversidad_query_field:"!biodiversidad_query"', array(
      '!biodiversidad_query_field' => 'PID',
      '!biodiversidad_query' => $pid,
    )
  );
  $query_processor->solrStart = 0;
  $query_processor->solrLimit = 1;
  $resultsonlythis = $solr->search($query_processor->solrQuery, $query_processor->solrStart, $query_processor->solrLimit, $query_processor->solrParams,  'GET');
  $jsononlythis = json_decode($resultsonlythis->getRawResponse(), TRUE);
  

}
catch (Exception $e) {
watchdog_exception('Islandora Red de Biodiversidad', $e, 'Got an exception while searching latlong for object in Solr ', array(), WATCHDOG_ERROR);
}


$list = array();
$locations = array();
$mapa_variables = array();
$lati_long = array();

$everyotherobject=array(); //to store temp all objects that don't have a location

$j=0;
$clustergroup='';
$jsonreversed=array();
foreach ($json['response']['docs'] as $key=>$choice) {
  $j++;
  if ((isset($choice[$mappings['location']['latlong_lat']])) && (isset($choice[$mappings['location']['latlong_long']]))) {
    //Inverse hash, we store the PID as key and the json response as value
    $jsonreversed[$choice['PID']]=$key;
    
    if (empty($limit_to_cmodel)) {
    //Only add all locations if we don't have a cmodel filter set by theme
    
      if ($choice['PID']==$pid) {
        $clustergroup='Current Object Location';	
      }
      else {
        $clustergroup='All locations in this Repository';	
      }	
    
    $point=array(
      'type' => 'point',
      'lat' => (float) $choice[$mappings['location']['latlong_lat']],
      'lon' => (float) $choice[$mappings['location']['latlong_long']],
      'leaflet_id' => 'point-'.$key,
      // 'flags'=>0x01,
      'popup' => "<span>Object of type $clustergroup:</span><br>".l($choice['fgs_label_mt'][0], $choice['PID_fullurl']).'<br>'.(float) $choice[$mappings['location']['latlong_lat']].",".(float) $choice[$mappings['location']['latlong_long']],
      'clusterGroup'=>$clustergroup,
    );
    //adding points to settings by clustergroup as clustergroup, means all
    array_push($locations, $point);	
    }
   
  }
}
//Now compute the computed/related objects according to their relations

$variables['wasgeocoded']=false;
$variables['geocoded']='hola';
foreach ($pathtopoints as $locationpid=>$objectsinpoint) {
if (array_key_exists($locationpid,$jsonreversed)) {
  
  //remove duplicates before adding
  if (count($everyotherobject)>0)
    {
      //objectsinpoint=array_merge($objectsinpoint,$everyotherobject);
    }
  
  $choice=$json['response']['docs'][$jsonreversed[$locationpid]];
  foreach ($objectsinpoint as $objectinpoint)
  {
    $objectinpoint['lat']=(float) $choice[$mappings['location']['latlong_lat']];
    $objectinpoint['lon']=(float) $choice[$mappings['location']['latlong_long']];
    $objectinpoint['popup']= $objectinpoint['popup'].'<br>'.(float) $choice[$mappings['location']['latlong_lat']].",".(float) $choice[$mappings['location']['latlong_long']];
    //adding points to settings by cmodel as clustergroup
    array_push($locations, $objectinpoint);	
  }
}
else {
//Means we don't have this location in our Solr points query 
//So lets call our geodecoder and get the info from there using what we got from solr for the current location object
  
  $query_processor->solrQuery = format_string(
   '!biodiversidad_query_field:"!biodiversidad_query"', array(
     '!biodiversidad_query_field' => 'PID',
     '!biodiversidad_query' => $locationpid,
   )
 );
 $query_processor->solrStart = 0;
 $query_processor->solrLimit = 1;
 $resultsonlythis = $solr->search($query_processor->solrQuery, $query_processor->solrStart, $query_processor->solrLimit, $query_processor->solrParams,  'GET');
 $jsononlythis = json_decode($resultsonlythis->getRawResponse(), TRUE);
  
  
  
  
  if ($jsononlythis['response']['numFound']>=1)
    { 
      
      $geodecodeparam[]=$jsononlythis['response']['docs'][0]['dwc.locality_s'];
      $geodecodeparam[]=$jsononlythis['response']['docs'][0]['dwc.municipality_s'];
      $geodecodeparam[]=$jsononlythis['response']['docs'][0]['dwc.county_s'];
      $geodecodeparam[]=$jsononlythis['response']['docs'][0]['dwc.stateProvince_s'];
      $geodecodeparam[]=$jsononlythis['response']['docs'][0]['dwc.country_s'];
      $geodecoded=islandora_leaflet_nominatimgeocoding(array_filter($geodecodeparam)); 
      if (count($geodecoded)>0)
      {
        $variables['wasgeocoded']=true;
        $variables['geodecoded']="";
        $rows=array();
        $rows2=array();       
        $message="<p>".t("Lat/long data was not available for object "). $jsononlythis['response']['docs'][0]['fgs_label_mt'][0];
        $message.="<br>".t("so we geodecoded on the fly it's metadata for display purpouses")."</p>";
        drupal_set_message($message,'status',FALSE);
        foreach ($geodecoded as $key=>$value)
          {
            if ($key=='display_name') {
              $variables['geodecoded'].="<h4>$value</h4>";
            }
            else {
              switch ($key) {
                case 'address':
                foreach($value as $desc=>$info)
                  {
                    $rows2[]=array($desc,$info);
                  }
                break;
                case 'boundingbox':
                  $rows[]=array($key,implode(";",$value));
                break;
                default: $rows[]=array($key,$value); 
              }
            }
          }
          $variables['geodecoded'].="<h2>".t("Geocoded data for ").$jsononlythis['response']['docs'][0]['fgs_label_mt'][0]."</h2><br>";
          if (count($rows)>0) {
            
          $variables['geodecoded'].=theme('table', array('header' => array(), 'rows' => $rows))."<br>";
          }
          if (count($rows2)>0) {
          $variables['geodecoded'].="<h2>".t('Address')."</h2>".theme('table', array('header' => array(), 'rows' => $rows2));
          }
          if ((count($everyotherobject)>0) && $current_is_location) {
            $objectsinpoint=array_merge($objectsinpoint,$everyotherobject);
          }
        foreach ($objectsinpoint as $objectinpoint) {
          $objectinpoint['lat']=(float) $geodecoded['lat'];
          $objectinpoint['lon']=(float) $geodecoded['lon'];
          $objectinpoint['popup']= $objectinpoint['popup'].'<br>'.(float) $geodecoded['lat'].",".(float) $geodecoded['lat'].'<br>'.t('This location was geocoded from metadata so only approximate');
          //adding points to settings by cmodel as clustergroup
          array_push($locations, $objectinpoint);	
        } 
      }
    }   
  }  
}




	
if (!empty($locations)) {
  $styles = module_invoke_all('leaflet_map_info'); 
  $variables['results'] =leaflet_render_map($styles['islandora_leaflet'], $locations, '500px');	
  }  
	
}
