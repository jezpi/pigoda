function reloadontm() {
     window.location.reload();
     window.setTimeout(reloadontm, 60000);
}
window.setTimeout(reloadontm, 60000);

