<?php
/**
* @file utils.inc
* This file part of Islandora Leaflet.
*
*  (c) 2017. Diego Pino Navarro <dpino@metro.org>
*  https://github.com/mnylc/islandora_leaflet.
*
*  This source file is subject to the GPL V 3.0 license that is bundled
*  with this source code and at
*  https://github.com/mnylc/islandora_leaflet/blob/master/LICENSE.
*/

function islandora_leaflet_nominatimgeocoding(array $searchparams) {        
  $geolocated=array();
  if (count($searchparams)==0) {
    return array();
  }
  $arguments=implode(",", array_map('urlencode',$searchparams));
  //open.mapquestapi.com/nominatim V1 API for geocoding
  $url ="http://open.mapquestapi.com/nominatim/v1/search.php";
  $params = array(
    'format' => 'json',
    'limit' => 3,
    'addressdetails' => 1,
    'osm_type' => 'N',
  );
      
      
    
  $response = drupal_http_request($url . "?q=$arguments&" . drupal_http_build_query($params));
     
  if (!empty($response->error)) {
    $msg_args = array(
      '%url' => $url,
      '@code' => $response->code,
      '%error' => $response->error,
    );
    watchdog('islandora_leaflet','the HTTP request %url returned the following error (code @code): %error.', $msg_args, WATCHDOG_ERROR);
    return FALSE;
  }
  $data = drupal_json_decode($response->data);
    
  $nodefound=FALSE;
  if (count($data))
  {
    foreach($data as $key=>$found)
    {
      if ($found['osm_type']=='node') 
      {
        //Take a node osm_type
        $geolocated['boundingbox']= $data[$key]['boundingbox'];
        $geolocated['licence']=$data[$key]['licence'];
        $geolocated['lat']= $data[$key]['lat'];
        $geolocated['lon'] = $data[$key]['lon'];
        $geolocated['display_name'] = $data[$key]['display_name'];
        $geolocated['address']= $data[$key]['address'];
        $nodefound=TRUE;
        break;
      } 
    }
    if (!$nodefound)
    {
         
      // I will just take the first one, whatever it is.
      $geolocated['boundingbox']= $data[0]['boundingbox'];
      $geolocated['licence']=$data[0]['licence'];
      $geolocated['lat']= $data[0]['lat'];
      $geolocated['lon'] = $data[0]['lon'];
      $geolocated['display_name'] = $data[0]['display_name'];
      $geolocated['address']= $data[0]['address'];
    }
  }
     
  return $geolocated;
    
}  