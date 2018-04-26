<?php
session_start();

if(isset($_SESSION['user'])=="")
{
 header("Location: index.php");
}
$play_path="user_info/user_".$_SESSION['user']."/video/".$_GET["play_id"];
//echo "user_info/user_".$_SESSION['user']."/video/".$_GET["play_id"];
//sleep(3);
$con = new mysqli('localhost','root', '4817','web');
if (!$con)
 {
 die('Could not connect: ' . mysql_error());
 }
 $con->set_charset("utf8");//或者 $mysqli->query("set names 'utf8'")
//mysql_select_db("ajax_demo", $con);
$sql="SELECT video_name FROM video_info WHERE user_id = '".$_SESSION['user']."'";
// echo "111";
//echo $row['air_condition_id'];
$res = $con->query($sql);
?>

<!DOCTYPE html>
<html lang="utf-8">
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<head>
</head>
<title>视频显示</title>
<body style="text-align:center;">
<p>
	<video width="300" height="300" controls="controls" preload="metadata"> 
	<source src=<?php echo($play_path)?> type="video/mp4"> 
		您的浏览器不支持此种视频格式。 
</video>
	</p>	
以下是你家里最近的视频缓存，点击以下某个时间查看详细的视频,或者
<a href="logout.php"><button>退出当前的用户登录？</button></input></a>:
<p>
 <?php   
while($row=$res->fetch_array())
 {
    echo "<a href= video.php?play_id=".$row["video_name"].">".$row["video_name"]."</a> ";
 }

$con->close();
?>
</p>
</body>  

</html>