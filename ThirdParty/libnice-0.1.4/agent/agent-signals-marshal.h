
#ifndef __agent_marshal_MARSHAL_H__
#define __agent_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:UINT,UINT,UINT (agent-signals-marshal.list:2) */
extern void agent_marshal_VOID__UINT_UINT_UINT (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);

/* VOID:UINT,UINT,STRING,STRING (agent-signals-marshal.list:4) */
extern void agent_marshal_VOID__UINT_UINT_STRING_STRING (GClosure     *closure,
                                                         GValue       *return_value,
                                                         guint         n_param_values,
                                                         const GValue *param_values,
                                                         gpointer      invocation_hint,
                                                         gpointer      marshal_data);

/* VOID:UINT,UINT,STRING (agent-signals-marshal.list:6) */
extern void agent_marshal_VOID__UINT_UINT_STRING (GClosure     *closure,
                                                  GValue       *return_value,
                                                  guint         n_param_values,
                                                  const GValue *param_values,
                                                  gpointer      invocation_hint,
                                                  gpointer      marshal_data);

/* VOID:UINT (agent-signals-marshal.list:9) */
#define agent_marshal_VOID__UINT	g_cclosure_marshal_VOID__UINT

/* VOID:UINT,UINT (agent-signals-marshal.list:11) */
extern void agent_marshal_VOID__UINT_UINT (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);

G_END_DECLS

#endif /* __agent_marshal_MARSHAL_H__ */

