<?php
// Initialize Smarty.

$topdir = realpath(".");
include 'smarty/Smarty.class.php';
$smarty = new Smarty;
$smarty->setTemplateDir($topdir . '/smarty/templates');
$smarty->setCompileDir($topdir . '/smarty/templates_c');
$smarty->setCacheDir($topdir . '/smarty/cache');
$smarty->setConfigDir($topdir . '/smarty/configs');

// Load config and assign it to Smarty variables.
include 'config.php';
$smarty->assign('phpbase', $WEB_BASEPHP);
$smarty->assign('cgibase', $WEB_BASECGI);
$smarty->assign('hotelgwaddress', $HOTELGW_ADDRESS);
$smarty->assign('hotelgwport', $HOTELGW_PORT);
$smarty->assign('paperaddress', $PAPER_ADDRESS);
$smarty->assign('paperport', $PAPER_PORT);

$err = 'No error';
$todisplay = "hotelform.html";

$smarty->assign('result', "No error");

if (!empty($_GET)) {
	if (isset($_GET['reason'])) {
		$smarty->assign('error', $_GET['reason']);
		$todisplay = "error.html";
	} else if (isset($_GET['res'])) {
		$smarty->assign('res', $_GET['res']);
		$todisplay = "result.html";

	} else {
		if ((isset($_GET['type'])) && (isset($_GET['name'])) && $_GET['name'] != "") {
			if ($socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP)) {
				if (socket_connect($socket, $HOTELGW_ADDRESS, $HOTELGW_PORT)) {
					socket_read($socket, 2048);
					$req = "b " . $_GET['type'] . " " . $_GET['name'] . "\n";
					socket_write($socket, $req);
					$res = trim(socket_read($socket, 2048));

					$smarty->assign('result', $res);

					socket_close($socket);
				} else {
					$err = "Cannot connect with addr $HOTELGW_ADDRESS:$HOTELGW_PORT!\n";
				}
			} else {
				$err = "Can't open connection to the server";
			}
		} else {
			if ((isset($_GET['type']) && $_GET['type'] != "") && (isset($_GET['name']) && $_GET['name'] == "")) {
				$err = "No name inserted!";
			} else {
				if ((!isset($_GET['type']) && $_GET['type'] == "") && (isset($_GET['name']) && $_GET['name'] != "")) {
					$err = "No room type inserted!";
				} else {
					$err = "No input inserted!";
				}

			}
		}
	}
}

if ($err != "No error") {
	header("Location: hotelbook.php?reason=" . $err);
	die();
} else if ($res == "ok") {
	$res = "Successfully booked room " . $_GET["type"];
	header("Location: hotelbook.php?res=" . $res);
	die();} else if ($res == "NA") {
	$res = "No available rooms of selected type!";
	header("Location: hotelbook.php?res=" . $res);
	die();
}

$smarty->display('extends:tpl/' . $todisplay);

?>
