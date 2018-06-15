void status_event_subscribe(uint64_t channel_id) {
  StatusInfo **ref = (StatusInfo **)pmap_ref(uint64_t)(xchannels, channel_id, true);
  if (!*ref) {
    *ref = xmalloc(sizeof(**ref));
  }
  StatusInfo *status = *ref;
  status->last_curbuf = -1;
  status->last_curwin = -1;
  status->last_curtab = -1;
}

void status_event_unsubscribe(uint64_t channel_id) {
  StatusInfo *status = pmap_del(uint64_t)(xchannels, channel_id);
  if (status) {
    xfree(status);
  }
}


static void update(uint64_t channel_id, StatusInfo *status) {
  Dictionary update = ARRAY_DICT_INIT;
  if (curbuf->handle != status->last_curbuf) {
    PUT(update, "curbuf", BUFFER_OBJ(curbuf->handle));
    status->last_curbuf = curbuf->handle;
  }
  if (curwin->handle != status->last_curwin) {
    PUT(update, "curwin", BUFFER_OBJ(curwin->handle));
    status->last_curwin = curwin->handle;
  }
  if (curtab->handle != status->last_curtab) {
    PUT(update, "curtab", BUFFER_OBJ(curtab->handle));
    status->last_curtab = curtab->handle;
  }
  if (update.size > 0) {
    Array args = ARRAY_DICT_INIT;
    ADD(args, DICTIONARY_OBJ(update));
    rpc_send_event(channel_id, "nvim_status", args);
  }
}


void status_event_update(uint64_t channel_id) {
  StatusInfo *status = pmap_get(uint64_t)(xchannels, channel_id);
  if (status) {
    update(channel_id, status);
  }
}

void status_event_update_all(void) {
  uint64_t channel_id;
  StatusInfo *status;
  map_foreach(xchannels, channel_id, status, {
    update(channel_id, status);
  });
}


