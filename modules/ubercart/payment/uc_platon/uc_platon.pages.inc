<?php

/**
 * @file
 * Platon menu items.
 *
 */
function uc_platon_complete($cart_id = 0) {
    watchdog('Platon', 'Receiving new order notification for order !order_id.', array('!order_id' => check_plain($_POST['order'])));

    $order = uc_order_load($_POST['order']);

    if ($order === FALSE) {
        watchdog('Platon', 'Wrong order !order_id.', array('!order_id' => check_plain($_POST['order'])), WATCHDOG_ERROR);
        die('Wrong order');
    }

    // generate signature from callback params
    $sign = md5(
            strtoupper(
                    strrev($_POST['email']) .
                    variable_get('uc_platon_password', '') .
                    $_POST['order'] .
                    strrev(substr($_POST['card'], 0, 6) . substr($_POST['card'], -4))
            )
    );

    // verify signature
    if ($_POST['sign'] !== $sign) {
        watchdog('Platon', 'Bad signature for order !order_id.', array('!order_id' => check_plain($_POST['order'])), WATCHDOG_ERROR);
        die("ERROR: Bad signature");
    }

    switch ($_POST['status']) {
        case 'SALE':
            $comment = t('Paid by Platon, external order ID: !order', array('!order' => check_plain($_POST['id'])));
            uc_payment_enter($order->order_id, 'platon', $_POST['amount'], 0, NULL, $comment);
            uc_cart_complete_sale($order, variable_get('uc_new_customer_login', FALSE));
            uc_order_comment_save($order->order_id, 0, t('Payment from Platon was received'), 'admin');
            watchdog('Platon', 'Payment for order !order_id complete.', array('!order_id' => check_plain($_POST['order'])));
            break;
        case 'REFUND':
            $status = 'canceled';
            // Save changes to order without it's completion.
            $change = array(t('Order status') => array('old' => uc_order_status_data($order->order_status, 'title'), 'new' => uc_order_status_data($status, 'title')));
            uc_order_log_changes($order->order_id, $change);
            $order->order_status = $status;
            uc_order_save($order);
            uc_order_comment_save($order->order_id, 0, t('Payment was refunded'), 'admin');
            watchdog('Platon', 'Payment for order !order_id refunded.', array('!order_id' => check_plain($_POST['order'])));
            break;
        case 'CHARGEBACK':
            break;
        default:
            die("ERROR: Invalid callback data");
    }
    exit("OK");
}


function uc_platon_return($cart_id = 0) {
    if(!$cart_id)
        $cart_id = uc_cart_get_id();
    
    // Empty that cart...
    uc_cart_empty($cart_id);
    $page = variable_get('uc_cart_checkout_complete_page', '');

    if (empty($page)) {
        $page = url('', array('absolute' => TRUE));
    }
    drupal_goto($page);
}
